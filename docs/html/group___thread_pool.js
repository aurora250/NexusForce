var group___thread_pool =
[
    [ "task_group", "structtask__group.html", [
      [ "decrement", "structtask__group.html#af957b2f16052c7bcc23e368b8fbaaaf0", null ],
      [ "increment", "structtask__group.html#aaece5444af599c96006158a557d2e5a9", null ],
      [ "wait", "structtask__group.html#aedc47e03d42c0117ef6634cec9e4466d", null ],
      [ "running_count", "structtask__group.html#ae2ea1d4a2ba691702834e2cf85c15a97", null ]
    ] ],
    [ "local_queue", "classlocal__queue.html", [
      [ "steal_strategy", "classlocal__queue.html#aae2f178f553c35b160ea3f5c6a8f1f16", [
        [ "half", "classlocal__queue.html#aae2f178f553c35b160ea3f5c6a8f1f16a7afe399f1415b137d0962f82662fa9d4", null ],
        [ "fixed_batch", "classlocal__queue.html#aae2f178f553c35b160ea3f5c6a8f1f16ad9c03a7a5c46b68317c3634254fad0d8", null ],
        [ "single", "classlocal__queue.html#aae2f178f553c35b160ea3f5c6a8f1f16add5c07036f2975ff4bce568b6511d3bc", null ],
        [ "adaptive", "classlocal__queue.html#aae2f178f553c35b160ea3f5c6a8f1f16a8cb043b2dace9afc0680e6bae5cd316f", null ]
      ] ],
      [ "be_stolen_by", "classlocal__queue.html#aa2f9e7f7671a4d2d80459a83a58133a7", null ],
      [ "capacity", "classlocal__queue.html#a4ea666df5cf1168666dba318973be5fe", null ],
      [ "empty", "classlocal__queue.html#a9d2eb136a42294cfa0451dce7d500ee0", null ],
      [ "push_back", "classlocal__queue.html#a70e2ab3d3d65f423ffe571788f3ca816", null ],
      [ "remain_size", "classlocal__queue.html#af84b6ab5fed4a7fac3b1ae7ead50bcf6", null ],
      [ "size", "classlocal__queue.html#a479ae3faa9c13754d8af5121e659e854", null ],
      [ "try_pop", "classlocal__queue.html#a4a7dfd98b0eceb8f752a61a2573c8881", null ]
    ] ],
    [ "worker_context", "structworker__context.html", [
      [ "id_type", "structworker__context.html#a9819ca5bea261e9484d75f512fc38326", null ],
      [ "consecutive_idle_count", "structworker__context.html#aa4cb896c147fd86efed7b3f176e8a233", null ],
      [ "id", "structworker__context.html#a0ce42d0b767acc53e81ba0e89aa4e3c4", null ],
      [ "is_stealing", "structworker__context.html#a0708436cfb366b691690600be934454e", null ],
      [ "queue", "structworker__context.html#a3d472b2df48065b84ccdb033671a352c", null ]
    ] ],
    [ "task_info", "structtask__info.html", [
      [ "status", "structtask__info.html#a82e511d7d330d103ad92f8acf75682ec", [
        [ "pending", "structtask__info.html#a82e511d7d330d103ad92f8acf75682eca7c6c2e5d48ab37a007cbf70d3ea25fa4", null ],
        [ "running", "structtask__info.html#a82e511d7d330d103ad92f8acf75682eca75101dcdfc88455bcafc9e53e0b06689", null ],
        [ "completed", "structtask__info.html#a82e511d7d330d103ad92f8acf75682ecaaa8fb77e57d1ca18d593e909729871fe", null ],
        [ "failed", "structtask__info.html#a82e511d7d330d103ad92f8acf75682eca26934eb377001f66e37289a5c93fe284", null ]
      ] ],
      [ "task_info", "structtask__info.html#a6d78f8c019063bc70f0677d80c8bdbee", null ],
      [ "exec_time", "structtask__info.html#aa8a3170428752bd2191733b36f28ee4f", null ],
      [ "is_finished", "structtask__info.html#a41b52cd02dff638ccf5ee23320a17e65", null ],
      [ "error", "structtask__info.html#aa98f5703a28f26d31912381175a3fc3d", null ],
      [ "finish_time", "structtask__info.html#ac00a351c9a460bf7500883f98541ba82", null ],
      [ "id", "structtask__info.html#a5dd1257311cea5dfb0f7ec96347cbdbd", null ],
      [ "priority", "structtask__info.html#abe9f70aa38bf6661803112370efc8a7e", null ],
      [ "start_time", "structtask__info.html#a16acfc5751d1fd71ef6bf34ae7658fe9", null ],
      [ "status", "structtask__info.html#a326631a3e74e507ba2cbe9118cb9e06f", null ],
      [ "submit_time", "structtask__info.html#a14d284999110a0f2049de841fc52b3fd", null ],
      [ "worker_thread_id", "structtask__info.html#ab1a355b154149595c310c3592e7ce9b6", null ]
    ] ],
    [ "submit_result&lt; T &gt;", "structsubmit__result.html", [
      [ "operator bool", "structsubmit__result.html#a1e7d1255b5e726d50bc742e63cdd4769", null ],
      [ "future", "structsubmit__result.html#a2aec5dabd1e7c8f5aa51c4aa01faaf09", null ],
      [ "task_info", "structsubmit__result.html#a936d7685d45adfd0eb8b2b820766047b", null ]
    ] ],
    [ "thread_pool", "classthread__pool.html", [
      [ "periodic_task_state", "structthread__pool_1_1periodic__task__state.html", [
        [ "cancelled", "structthread__pool_1_1periodic__task__state.html#abd5456c25461360f12c5cadb54c8d3cd", null ]
      ] ],
      [ "pool_statistics", "structthread__pool_1_1pool__statistics.html", [
        [ "to_string", "structthread__pool_1_1pool__statistics.html#a0e6c16f6569d8ab4029225048138c4d5", null ],
        [ "busy_threads", "structthread__pool_1_1pool__statistics.html#a22d0f279b8e8f1e13cc6c687f6614129", null ],
        [ "idle_threads", "structthread__pool_1_1pool__statistics.html#a516305a224c339966b894f6167437555", null ],
        [ "queue_size", "structthread__pool_1_1pool__statistics.html#a710ffce55e6a08a2e01063c25a1cdce8", null ],
        [ "total_completed", "structthread__pool_1_1pool__statistics.html#a12fcfec0718443ef61f73f7006618c77", null ],
        [ "total_stolen", "structthread__pool_1_1pool__statistics.html#a6dcf7760a734d79829fa49c24a6d6b3d", null ],
        [ "total_submitted", "structthread__pool_1_1pool__statistics.html#af23fd661d07ca3aede5ae96d47de07ff", null ],
        [ "total_threads", "structthread__pool_1_1pool__statistics.html#abf2cb7bccc5cd7e308bce4996093d35b", null ]
      ] ],
      [ "id_type", "classthread__pool.html#a11c3d03ee2141f13727cbd973c24134e", null ],
      [ "periodic_token", "classthread__pool.html#a441d12dda9edd4d083d844279f1ae91d", null ],
      [ "priority_type", "classthread__pool.html#a0f86abfce93a05b67165f166c653a82b", null ],
      [ "steal_strategy", "classthread__pool.html#a9177f65ba06f1f7371a69890ece9503d", null ],
      [ "pool_mode", "classthread__pool.html#a4c640f1911280a2e33221d6d2a8148d3", [
        [ "fixed", "classthread__pool.html#a4c640f1911280a2e33221d6d2a8148d3acec315e3d0975e5cc2811d5d8725f149", null ],
        [ "cached", "classthread__pool.html#a4c640f1911280a2e33221d6d2a8148d3a1fb1a060534164a18a99494122825190", null ]
      ] ],
      [ "thread_pool", "classthread__pool.html#a78324a6b46c41401389ef61d5f7d21a4", null ],
      [ "~thread_pool", "classthread__pool.html#ac9d8f108fa2419441aba432f8ca98eae", null ],
      [ "mode", "classthread__pool.html#a3b141f1ae010547460c9f9c0f5a7dfbf", null ],
      [ "running", "classthread__pool.html#a528ed99848dc5301853c4ab581c9d07d", null ],
      [ "set_mode", "classthread__pool.html#ad2570e71325f8fc0056e2b7db00c1369", null ],
      [ "set_steal_mode", "classthread__pool.html#af7af30cde703cd7de4e3484f16b29fe5", null ],
      [ "set_task_threshhold", "classthread__pool.html#a9cf841376ecf2272f396bc2a42b89200", null ],
      [ "set_thread_threshhold", "classthread__pool.html#a0f8735976acc53cc7b02517efe8ae23b", null ],
      [ "start", "classthread__pool.html#ae4af3da255dd5f55a7314e804cd91571", null ],
      [ "statistics", "classthread__pool.html#a3a7e731e56996f60b6a2a857f89c3eab", null ],
      [ "stop", "classthread__pool.html#a3679e32679c97ea0055a38f2df4560ec", null ],
      [ "submit_after", "classthread__pool.html#a3b33992c58dbd64f6c397fbc7092406c", null ],
      [ "submit_after", "classthread__pool.html#ae3bf04c2adddd193bba36f6c338c9b9e", null ],
      [ "submit_every", "classthread__pool.html#a4119fc5a49cda3741698ae66c653894d", null ],
      [ "submit_every", "classthread__pool.html#a159c162d139e452d7539a4d0c46fc908", null ],
      [ "submit_task", "classthread__pool.html#a1a93d040b178d96b993dffc135a52fb2", null ],
      [ "submit_task", "classthread__pool.html#a4269d4f0985eeb9e6f38bb9aeae9e67a", null ]
    ] ],
    [ "get_current_task_group", "group___thread_pool.html#ga00d1068c932ecf58f7f1887c2e27844f", null ],
    [ "get_worker_context", "group___thread_pool.html#gabcf2301dd14a385109c6ba1514357ddd", null ]
];