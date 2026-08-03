var group___daemon =
[
    [ "neforce::daemon", "classneforce_1_1daemon.html", [
      [ "child_config", "structneforce_1_1daemon_1_1child__config.html", [
        [ "args", "structneforce_1_1daemon_1_1child__config.html#a0c7c1ab7727723968b1afea024cbb951", null ],
        [ "capture_output", "structneforce_1_1daemon_1_1child__config.html#a732036dcbc3d3cdb3dc978b9f7ae564e", null ],
        [ "envs", "structneforce_1_1daemon_1_1child__config.html#a57254113e3d45abd5ebd93cec0463abe", null ],
        [ "executable", "structneforce_1_1daemon_1_1child__config.html#abbad3078f1a7a580b6b451ff7439bc6d", null ],
        [ "graceful_timeout_ms", "structneforce_1_1daemon_1_1child__config.html#a39158855ca6a1e1a4b2cd2fdeff8fb11", null ],
        [ "health_check_interval_ms", "structneforce_1_1daemon_1_1child__config.html#ab8626681a3bb09ee071b7a15aebaa3b6", null ],
        [ "max_restarts", "structneforce_1_1daemon_1_1child__config.html#a53b94a05439d4716f3c9835fe51eba6a", null ],
        [ "name", "structneforce_1_1daemon_1_1child__config.html#a79fc22a35220a343a16db7ac9570bb76", null ],
        [ "restart_delay_ms", "structneforce_1_1daemon_1_1child__config.html#a01b9f85d82d0104cd8336931b1eeab3a", null ],
        [ "work_dir", "structneforce_1_1daemon_1_1child__config.html#a07c23dcb325ca16718c22ff0525647d9", null ]
      ] ],
      [ "child_status", "structneforce_1_1daemon_1_1child__status.html", [
        [ "exit_code", "structneforce_1_1daemon_1_1child__status.html#ab5b0405efa5e56c2fe467b29622a467d", null ],
        [ "name", "structneforce_1_1daemon_1_1child__status.html#a5468455cbf235b4b4095cb5d5cf24b01", null ],
        [ "pid", "structneforce_1_1daemon_1_1child__status.html#a0335b2d08a65b37cd3908f5c61c37461", null ],
        [ "restart_count", "structneforce_1_1daemon_1_1child__status.html#a1414db056247b175150b871d222e06a5", null ],
        [ "running", "structneforce_1_1daemon_1_1child__status.html#ad3cf430938fb4d501b73f0cb6b73cb74", null ]
      ] ],
      [ "child_exit_callback", "classneforce_1_1daemon.html#a1b9149d3fe6e7562d4fad60f615d8c05", null ],
      [ "reload_callback", "classneforce_1_1daemon.html#a7d202afa1e3ddc7db9c1b7075e9c5183", null ],
      [ "start_callback", "classneforce_1_1daemon.html#ad9c095abfc5ac5c50d666438c5598fe4", null ],
      [ "stop_callback", "classneforce_1_1daemon.html#ab82c80661626912f276452047a3709b5", null ],
      [ "daemon_state", "classneforce_1_1daemon.html#a200a37191cdcfc3d52691b5cff27e98f", [
        [ "stopped", "classneforce_1_1daemon.html#a200a37191cdcfc3d52691b5cff27e98faf0a0bfe6bc7d2c58d2989034f83183e0", null ],
        [ "starting", "classneforce_1_1daemon.html#a200a37191cdcfc3d52691b5cff27e98fa1ee85f6c60017a7f0646ba8dc5824de6", null ],
        [ "running", "classneforce_1_1daemon.html#a200a37191cdcfc3d52691b5cff27e98fa75101dcdfc88455bcafc9e53e0b06689", null ],
        [ "reloading", "classneforce_1_1daemon.html#a200a37191cdcfc3d52691b5cff27e98fa588835865e53c295f44581aabf98501b", null ],
        [ "stopping", "classneforce_1_1daemon.html#a200a37191cdcfc3d52691b5cff27e98fa3b2a3c8ed19fc3647432e72885d633e7", null ]
      ] ],
      [ "add_child", "classneforce_1_1daemon.html#a8994d40b17ba8e81be99f229a2cc48e9", null ],
      [ "all_child_statuses", "classneforce_1_1daemon.html#a87d9d9ed587850d378321bf40d43be20", null ],
      [ "daemonize", "classneforce_1_1daemon.html#a39b3b361534c0b20a3a6e63f7de7fc0e", null ],
      [ "get_child_status", "classneforce_1_1daemon.html#abf4437c99e6d6098df15e4446e277e55", null ],
      [ "on_child_exit", "classneforce_1_1daemon.html#a38c04dd4bd273d1ebc3ccdf0a48cf244", null ],
      [ "on_reload", "classneforce_1_1daemon.html#a83393c0784e03b3354397bb5049f05db", null ],
      [ "on_start", "classneforce_1_1daemon.html#a9419ee7d6f6daf4436077d3094da6768", null ],
      [ "on_stop", "classneforce_1_1daemon.html#a92cfbac519dad62a8c87c34e9704eec8", null ],
      [ "remove_child", "classneforce_1_1daemon.html#a7759a95cd155b0a98724b0d422e2568c", null ],
      [ "remove_pid_file", "classneforce_1_1daemon.html#a540c7f863f73d94535ea7f70af70a16f", null ],
      [ "request_reload", "classneforce_1_1daemon.html#ad48c058c8a5b0fe3070ee352005b488f", null ],
      [ "request_shutdown", "classneforce_1_1daemon.html#a0b1df6708e98ef03c169aed4d3179923", null ],
      [ "run", "classneforce_1_1daemon.html#a2984d1f73e8ce6a2112d6dcb7a6e379b", null ],
      [ "set_watchdog_timeout", "classneforce_1_1daemon.html#aef53bb6b2fe5fce6ac3c362b78d8fceb", null ],
      [ "state", "classneforce_1_1daemon.html#a6aa28af536e9268dff8b37614165dcb0", null ],
      [ "watchdog_ping", "classneforce_1_1daemon.html#a6854e3e8efdf8567fe33692afbf3b2a3", null ],
      [ "write_pid_file", "classneforce_1_1daemon.html#a3f2c8fd4ac6a08d4d6be3fa50df80363", null ]
    ] ]
];