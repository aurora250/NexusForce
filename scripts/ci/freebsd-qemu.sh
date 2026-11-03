#!/usr/bin/env bash
#
# FreeBSD QEMU helpers shared by CI: prepare the VM image/seed, provision it once
# (disable the first-boot freebsd-update), boot it for tests and power it off.
#
# usage: freebsd-qemu.sh <x64|arm64> <prepare|provision|boot|start|wait|wait-root|poweroff> \
#            <vm-dir> <ssh-key> [ssh-port] [ssh-user]
#
set -euo pipefail

arch=${1:?architecture}
action=${2:?action}
vm_dir=${3:?vm directory}
key=${4:?ssh private key}
port=${5:-2222}
version=${FREEBSD_VERSION:-14.4}
mirror=${FREEBSD_MIRROR:-https://mirror.nju.edu.cn/freebsd}

case "$arch" in
    x64)
        qemu=qemu-system-x86_64
        image_arch=amd64
        image_name="FreeBSD-${version}-RELEASE-amd64-BASIC-CLOUDINIT-ufs.qcow2.xz"
        machine_args=(-machine q35 -accel tcg,thread=multi -cpu qemu64 -smp 2 -m 2048
                      -object rng-random,filename=/dev/urandom,id=rng0
                      -device virtio-rng-pci,rng=rng0)
        ;;
    arm64)
        qemu=qemu-system-aarch64
        image_arch=aarch64
        image_name="FreeBSD-${version}-RELEASE-arm64-aarch64-BASIC-CLOUDINIT-ufs.qcow2.xz"
        machine_args=(-machine virt -accel tcg,thread=multi -cpu max -smp 2 -m 2048
                      -object rng-random,filename=/dev/urandom,id=rng0
                      -device virtio-rng-pci,rng=rng0)
        ;;
    *)
        echo "unsupported architecture: $arch" >&2
        exit 1
        ;;
esac

ssh_opts=(-i "$key" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null
          -o LogLevel=ERROR -o ConnectTimeout=5)

wait_ssh() {
    local user=${1:-tester} tries=${2:-240}
    for ((i = 0; i < tries; i++)); do
        if ssh "${ssh_opts[@]}" -p "$port" "$user"@127.0.0.1 true 2>/dev/null; then
            return 0
        fi
        sleep 5
    done
    echo "ssh ($user) did not come up on port $port" >&2
    tail -n 120 "$vm_dir/serial.log" >&2 || true
    return 1
}

prepare() {
    mkdir -p "$vm_dir"
    cd "$vm_dir"

    if [ ! -f disk.qcow2 ]; then
        curl -fsSL \
            "$mirror/releases/VM-IMAGES/${version}-RELEASE/${image_arch}/Latest/${image_name}" \
            -o image.xz
        unxz -f image.xz
        mv image disk.qcow2
        qemu-img resize disk.qcow2 +16G
    fi

    if [ ! -f seed.iso ]; then
        if [ ! -f id_ed25519 ]; then
            ssh-keygen -t ed25519 -N '' -f id_ed25519 >/dev/null
        fi
        local pub
        pub=$(cat id_ed25519.pub)
        cat > user-data <<CLOUD_CONFIG
#cloud-config
hostname: nf-ci-${arch}
users:
  - name: tester
    groups: wheel
    shell: /bin/sh
    lock_passwd: false
    plain_text_passwd: tester
    ssh_authorized_keys:
      - ${pub}
ssh_pwauth: true
disable_root: false
chpasswd:
  expire: false
  list: |
    root:tester
runcmd:
  - install -d -m 700 /root/.ssh
  - install -m 600 -o root -g wheel /home/tester/.ssh/authorized_keys /root/.ssh/authorized_keys
  - grep -q '^PermitRootLogin' /etc/ssh/sshd_config || echo 'PermitRootLogin yes' >> /etc/ssh/sshd_config
  - sysrc firstboot_freebsd_update_enable=NO
  - sysrc sshd_enable=YES
  - ssh-keygen -A
  - service sshd restart
  - rm -f /firstboot
CLOUD_CONFIG
        printf 'instance-id: nf-ci-%s\nlocal-hostname: nf-ci-%s\n' "$arch" "$arch" > meta-data
        cloud-localds seed.iso user-data meta-data
    fi

    if [ "$arch" = arm64 ] && [ ! -f AAVMF_VARS.fd ]; then
        cp -f /usr/share/AAVMF/AAVMF_VARS.fd AAVMF_VARS.fd
    fi
}

qemu_command() {
    local dns=${1:-}
    local net="user,id=n0,hostfwd=tcp:127.0.0.1:${port}-:22"
    if [ -n "$dns" ]; then
        net="$net,dns=$dns"
    fi

    local args=("${machine_args[@]}")
    if [ "$arch" = arm64 ]; then
        args+=(
            -drive if=pflash,format=raw,readonly=on,file=/usr/share/AAVMF/AAVMF_CODE.fd
            -drive if=pflash,format=raw,file="$vm_dir/AAVMF_VARS.fd"
        )
    fi

    rm -f "$vm_dir/serial.log"
    "$qemu" "${args[@]}" \
        -drive "file=$vm_dir/disk.qcow2,if=virtio,cache=unsafe" \
        -drive "file=$vm_dir/seed.iso,media=cdrom,readonly=on" \
        -netdev "$net" -device virtio-net-pci,netdev=n0,romfile= \
        -display none -serial "file:$vm_dir/serial.log" -daemonize
}

poweroff() {
    ssh "${ssh_opts[@]}" -p "$port" root@127.0.0.1 'sync; shutdown -p now' 2>/dev/null || true
    for ((i = 0; i < 90; i++)); do
        if ! pgrep -f "${vm_dir}/disk.qcow2" >/dev/null; then
            return 0
        fi
        sleep 2
    done
    pkill -f "${vm_dir}/disk.qcow2" || true
}

provision() {
    # Black-hole DNS on the first boot: the image's first-boot freebsd-update
    # would otherwise stall for many minutes before the seed's runcmd executes.
    qemu_command 127.0.0.1
    wait_ssh root
    ssh "${ssh_opts[@]}" -p "$port" root@127.0.0.1 \
        'sysrc firstboot_freebsd_update_enable=NO; rm -f /firstboot; rm -rf /var/db/freebsd-update; sync'
    poweroff
}

case "$action" in
    prepare) prepare ;;
    provision)
        prepare
        provision
        ;;
    boot)
        qemu_command
        ;;
    start)
        prepare
        qemu_command
        ;;
    wait)
        wait_ssh "${6:-tester}"
        ;;
    wait-root)
        wait_ssh root
        ;;
    poweroff)
        poweroff
        ;;
    *)
        echo "unknown action: $action" >&2
        exit 1
        ;;
esac
