# 修复 DeepSeek Harness 留下的工作区 ACL 状态：
#   * 完整性标签被降到 Low（导致普通工具无法写 %TEMP%、文件读写被拒）
#   * 根目录被加入 S-1-4-* / S-1-15-2-* 包 SID 授予与 Everyone 的拒绝 ACE
#   * 继承被关闭
# pwsh -NoProfile -File D:\Workspace\NexusForce\scripts\fix_workspace_acl.ps1
# 该脚本幂等：重复执行不会产生额外变化。

[CmdletBinding()]
param(
    [string]$Path = ''
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrEmpty($Path)) {
    $Path = Split-Path -Parent $PSScriptRoot
}

if (-not (Test-Path -LiteralPath $Path)) {
    Write-Error "path not found: $Path"
    exit 1
}

$changes = @()

function Get-IntegrityLabel {
    param([string]$Target)
    return ((icacls $Target | Select-String -Pattern 'Mandatory Label' | ForEach-Object { $_.Line.Trim() }) -join ';')
}

# 1. 完整性标签恢复为 Medium，并向下继承
$before = Get-IntegrityLabel $Path
icacls $Path /setintegritylevel '(OI)(CI)M' | Out-Null
$after = Get-IntegrityLabel $Path
if ($before -ne $after) {
    $changes += "integrity: $before -> $after"
}

# 2. 移除 harness 安装的包 SID 授予与 Everyone 拒绝项
try {
    $acl = Get-Acl -LiteralPath $Path
    $dirty = $false

    foreach ($ace in @($acl.Access)) {
        $identity = $ace.IdentityReference.Value

        if ($identity -like 'S-1-4-*' -or $identity -like 'S-1-15-2-*') {
            $sid = New-Object System.Security.Principal.SecurityIdentifier($identity)
            $acl.PurgeAccessRules($sid)
            $changes += "removed package grant: $identity"
            $dirty = $true
        } elseif ($identity -eq 'Everyone' -and $ace.AccessControlType -eq 'Deny') {
            $acl.RemoveAccessRule($ace) | Out-Null
            $changes += "removed deny ACE: Everyone"
            $dirty = $true
        }
    }

    if ($dirty) {
        Set-Acl -LiteralPath $Path -AclObject $acl
    }
} catch {
    $changes += "acl edit skipped: $($_.Exception.Message)"
}

# 3. 重新启用继承（让根目录回到父目录的默认权限）
icacls $Path /inheritance:e | Out-Null

if ($changes.Count -eq 0) {
    Write-Output "workspace ACL already healthy: $Path"
} else {
    Write-Output "workspace ACL repaired: $Path"
    $changes | ForEach-Object { Write-Output "  - $_" }
}

# 4. 验证
$label = Get-IntegrityLabel $Path
$package = (icacls $Path | Select-String -Pattern 'S-1-4-' | ForEach-Object { $_.Line.Trim() }) -join ';'

Write-Output "  label: $label"
Write-Output "  package ACEs: $(if ($package) { 'present' } else { 'none' })"

$probe = Join-Path $Path ('build\.acl_probe_' + [guid]::NewGuid().ToString('N') + '.tmp')
try {
    [System.IO.File]::WriteAllText($probe, 'ok')
    Remove-Item $probe -Force
    Write-Output '  write: ok'
} catch {
    Write-Output "  write: failed ($($_.Exception.Message))"
}
