# Instruction

Recommended viewer for this file: Typora.

## Revisions

- Test Set 1: `_*.sh` + `i2_venc_test??.sh`
- Test Set 2: `_*.sh` + `case??.sh`

The goal is to port from test set 1 to test set 2. If test case in Test Set 2 is presented, then the one on Test Set 2 will be maintained. For example, `case05.sh` is there, then `i2_venc_test05.sh` is regarded as obsoleted.


Test Set 2 should provide
- Fewer script lines to be written in each test case, more variable and function are collected in _test.sh
- Auto set up ES output path and system binary path.
- Better management of MD5 values for each CODEC and version. Typically `s_` is for standard version, `p_` is for PMBR-off version.
  e.g. p_nRow0, s_nRow0
- Similar test cases between H.264 & H.265 should be organized better with function without copy-and-paste style in Test Set 1.
- Warning function, `check()` is  provided to check but not considered as error. This could be useful for MD5 checking.

## Environment
`PATH=/config:/sbin:/usr/sbin:/bin:/usr/bin`

default PATH environment variable is `/sbin:/bin:/config`
Change $PATH to include `/usr` path, otherwise, some commands used in the script set could not be run correctly, such as `expr` and `basename`.
Since **Test Set 2**, `PATH` would be set automatically.

### Set other environment variable
`export mnt=THE_MOUNTED_SAMBA_PATH`

Typically it would be `/mnt` or `/vendor`
If the environment variable is not set, _config.sh would set a default value to `/mnt`.
Thus, if EVB mounted samba to `/mnt`, this step could be skipped. Now `_config.sh` would try to `grep` first mounted samba path automatically but it is only tested on i2 DVR only. Set your own path instead if the script failed to set `$mnt`.

### ES Output PATH
Elementary Stream `*.es` would be output to `$mnt/out/es`.

**Test Set 2:**
If that path is not there, the script would try to create the path automatically. The script should stop if the creation fails.
For example, EVB does not mount samba into `$mnt` yet.

## Configuration

### Align the answers with your test code

This test set assume that the input YUV frames are built in the test code. Change shell script variable `yuv` in `_config.sh`. If new input set is used, append your MD5 answers for the new input YUV frames in `_config.sh`.
If you'd like to test the performance, put YUV data into `/tmp` or use file cache for faster data filling.

### Other Settings

Review settings in `_config.sh` to set the name of out_path. Refer description in `_config.sh` for the detail.

## Test

- `./list.sh` lists all test cases of the latest test set.
  - `./list.sh 1` lists all test cases in Test Set 1
- Change  directory to script test case. Make sure that the shell could execute `feature_venc` or `venc` without path.
- There are 3 ways to test with this script set.
  - **Full Report**: `./summary.sh` to test **all** test cases with summary information. Recommended for tester/QA.
  - **Single Report**: `./summary.sh case02.sh ` to test only test case 2 **with** summary information. Recommend for feature developer who modifies c code only.
  - **Single Test**: `./case02.sh` to test only test case 2 **without** summary information. The test result spreads out in the terminal output for easier debugging. Recommended for script developer.

#Development

This section collects some random notes so far. It is based on developing QA and not well-organized yet.

## Unset

Some functions, such as `run_es_on_ok_check` would run `$unset_script` inside to clear many variables. This separates the variables between test cases. It is similar  to TEAR_DOWN fixture in XUNIT.

Script developer needs to write test variables ever time. like this.

````shell
codec=h264
export VENC_GOP=10; export VENC_RC=1; export RcType="Cbr"; export Bitrate=1000000
run_case1

codec=h265
export VENC_GOP=10; export VENC_RC=1; export RcType="Cbr"; export Bitrate=1000000
run_case2
````

Which might seems noisy but it gives better ideal of what to be tested. And the final test script might look like this:

````shell
set_cbr() {
    export VENC_GOP=10; export VENC_RC=1; export RcType="Cbr"; export Bitrate=1000000
}
codec=h264
set_cbr
run_case1

codec=h265
set_cbr
run_case2
````
