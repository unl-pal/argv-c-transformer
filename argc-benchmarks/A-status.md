### Benchmark Analysis Summary: Unknowns and Errors

This summary lists benchmark files and their associated properties where at least one verifier produced a result other than 'true' or 'false', excluding instances where both verifiers timed out.

### both TO
- **antirez_redis_localtime.yml** with property **termination**
- **antirez_redis_localtime_unsafe.yml** with property **termination**
- **antirez_redis_localtime.yml** with property **no-overflow**
- **antirez_redis_localtime_unsafe.yml** with property **no-overflow**
- **antirez_redis_localtime.yml** with property **unreach-call**
- **antirez_redis_localtime_unsafe.yml** with property **unreach-call**

- **scottmwinters_projects_sorts.yml** with property **unreach-call**
- **scottmwinters_projects_sorts_ins.yml** with property **unreach-call**
- **scottmwinters_projects_sorts_sel.yml** with property **unreach-call**
- **scottmwinters_projects_sorts.yml** with property **valid-memsafety**

- **antirez_redis_strl.yml** with property **unreach-call**

- **antirez_redis_mt19937-64_array.yml** with property **valid-memsafety**

#### visit-vis_VisIt_matrix.yml
- **valid-memsafety**:
  - CPAchecker: `TIMEOUT`
  - UAutomizer: `ERROR (7)`
- **unreach-call**:
  - CPAchecker: `OUT OF MEMORY`
  - UAutomizer: `TIMEOUT`
- **termination**:
  - CPAchecker: `TIMEOUT`
  - UAutomizer: `ERROR (7)`
- **no-overflow**:
  - CPAchecker: `ERROR (interpolation failed)`
  - UAutomizer: `TIMEOUT`

#### antirez_redis_fastjson.yml
- **valid-memsafety**:
  - CPAchecker: `true`
  - UAutomizer: `ERROR (7)`
- **unreach-call**:
  - CPAchecker: `TIMEOUT`
  - UAutomizer: `unknown`
- **termination**:
  - CPAchecker: `ERROR (recursion)`
  - UAutomizer: `TIMEOUT`
- **no-overflow**:
  - CPAchecker: `ERROR`
  - UAutomizer: `unknown`

#### plexinc_plex-home-theater-public_fstrcmp.yml
- **valid-memsafety**:
  - CPAchecker: `true`
  - UAutomizer: `OUT OF MEMORY`
- **unreach-call**:
  - CPAchecker: `ERROR`
  - UAutomizer: `unknown`
- **termination**:
  - CPAchecker: `ERROR (recursion)`
  - UAutomizer: `TIMEOUT`
- **no-overflow**:
  - CPAchecker: `ERROR (interpolation failed)`
  - UAutomizer: `unknown`

#### scottmwinters_projects_sorts_mer.yml
- **valid-memsafety**:
  - CPAchecker: `true`
  - UAutomizer: `TIMEOUT`
- **unreach-call**:
  - CPAchecker: `ERROR (interpolation failed)`
  - UAutomizer: `OUT OF MEMORY`
- **termination**:
  - CPAchecker: `ERROR (recursion)`
  - UAutomizer: `TIMEOUT`

#### visit-vis_VisIt_dehex.yml
- **valid-memsafety**:
  - CPAchecker: `false(valid-deref)`
  - UAutomizer: `unknown`
- **unreach-call**:
  - CPAchecker: `false(unreach-call)`
  - UAutomizer: `unknown`
- **termination**:
  - CPAchecker: `OUT OF MEMORY`
  - UAutomizer: `ERROR`

#### visit-vis_VisIt_enhex.yml
- **valid-memsafety**:
  - CPAchecker: `ERROR`
  - UAutomizer: `unknown`
- **unreach-call**:
  - CPAchecker: `TIMEOUT`
  - UAutomizer: `unknown`
- **termination**:
  - CPAchecker: `OUT OF MEMORY`
  - UAutomizer: `ERROR`

#### antirez_redis_mt19937-64.yml
- **termination**:
  - CPAchecker: `true`
  - UAutomizer: `TIMEOUT`

#### antirez_redis_mt19937-64_array.yml
- **termination**:
  - CPAchecker: `true`
  - UAutomizer: `TIMEOUT`

#### antirez_redis_strl.yml
- **valid-memsafety**:
  - CPAchecker: `true`
  - UAutomizer: `OUT OF MEMORY`
- **termination**:
  - CPAchecker: `ERROR`
  - UAutomizer: `TIMEOUT`

#### plexinc_plex-home-theater-public_rand_r.yml
- **unreach-call**:
  - CPAchecker: `true`
  - UAutomizer: `TIMEOUT`

#### scottmwinters_projects_sorts.yml
- **termination**:
  - CPAchecker: `ERROR (recursion)`
  - UAutomizer: `ERROR (7)`

#### scottmwinters_projects_sorts_bub.yml
- **unreach-call**:
  - CPAchecker: `TIMEOUT`
  - UAutomizer: `false(unreach-call)`
- **termination**:
  - CPAchecker: `EXCEPTION`
  - UAutomizer: `true`


#### DrKLO_Telegram_superfasthash.yml
- **termination**:
  - CPAchecker: `OUT OF MEMORY`
  - UAutomizer: `true`
