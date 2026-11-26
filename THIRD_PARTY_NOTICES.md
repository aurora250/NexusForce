# Third-Party Notices

NexusForce itself is distributed under the MIT License (see [LICENSE](LICENSE)).

This file reproduces the copyright notices and license texts of third-party
material that is **included in, or linked into, NexusForce binaries**. It exists
because the MIT License alone does not satisfy the attribution requirements of
the licenses below, which oblige distributors of *binary* forms (for example a
shipped `libNexusForce.so`, `NexusForce.dll` or `libNexusForce.dylib`) to
reproduce the notices as well. Keep this file alongside any binary distribution.

---

## 1. Ported source code

### moodycamel::ConcurrentQueue

`include/NeForce/core/async/lock_free_queue.hpp` is a port of
[moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue)
(2020 version), adapted to the NexusForce code style and components.

```
Copyright (c) 2013-2020, Cameron Desrochers.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

The original work is dual-licensed and is also available under the
[Boost Software License 1.0](https://www.boost.org/LICENSE_1_0.txt).

### Bit Twiddling Hacks

The round-up-to-power-of-two helper in
`include/NeForce/core/async/lock_free_queue.hpp` follows the technique published
in [Bit Twiddling Hacks](https://graphics.stanford.edu/~seander/bithacks.html)
by Sean Eron Anderson, which is released into the public domain. Attribution is
recorded here as a courtesy.

---

## 2. Linked third-party libraries

NexusForce links the libraries below. On platforms where they are linked
**statically** (the vcpkg default for several of them), their code is embedded in
NexusForce binaries and the corresponding license text must be reproduced by the
distributor. When they are linked dynamically, they are separate works and their
licenses apply to those libraries themselves.

| Library          | Used for                                 | License                                        |
|------------------|------------------------------------------|------------------------------------------------|
| OpenSSL          | TLS, X.509, crypto primitives            | Apache-2.0                                     |
| ICU (ICU4C)      | Locale and Unicode support               | Unicode-DFS-2016                               |
| zlib             | Deflate compression                      | Zlib                                           |
| LZ4              | Fast compression                         | BSD-2-Clause                                   |
| SQLite3          | Embedded database                        | Public Domain                                  |
| SQLCipher        | Encrypted SQLite                         | BSD-style (Zetetic LLC)                        |
| hiredis          | Redis client protocol                    | BSD-3-Clause                                   |
| libpq            | PostgreSQL client                        | PostgreSQL License                             |
| libmysql         | MySQL client                             | GPL-2.0-only WITH Universal-FOSS-exception-1.0 |
| GoogleTest       | Unit and integration tests (not shipped) | BSD-3-Clause                                   |
| Google Benchmark | Benchmarks (not shipped)                 | Apache-2.0                                     |

### Note on libmysql

The MySQL client library is distributed under **GPL-2.0-only with the Universal
FOSS Exception**, not a permissive license. It is an *optional* dependency
controlled by `NEXUSFORCE_ENABLE_MYSQL` (`config.json` → `dependency.mysql.switch`).
Anyone redistributing a NexusForce binary built with MySQL support enabled should
confirm that the intended distribution satisfies the GPL, or build with
`-DNEXUSFORCE_ENABLE_MYSQL=OFF`.

---

## 3. Standards and specifications

NexusForce implements behaviour described by the standards and RFCs referenced in
its header documentation (ISO/IEC 14882, ISO/IEC 9075, IEEE 754, POSIX, RFC 792,
RFC 1035, RFC 3339, RFC 4648, RFC 6265, RFC 6455, RFC 7540, RFC 7541, RFC 7692,
RFC 8259, RFC 8439, RFC 9110, RFC 9112, ECMA-404, YAML 1.2.2, W3C CSS Color 4).
Specifications are not copied into this project; the references are citations.
