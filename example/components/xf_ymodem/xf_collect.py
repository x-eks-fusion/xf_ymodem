import xf_build

srcs = [
    "../../../*.c",
]

incs = [
    "../../..",
    "../../../config"
]

xf_build.collect(srcs=srcs, inc_dirs=incs)
