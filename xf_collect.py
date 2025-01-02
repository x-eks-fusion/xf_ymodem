import xf_build

srcs = [
    "*.c",
]

incs = [
    ".",
    "config",
]

reqs = [
    "xf_utils",
    "xf_sys",
]

cflags = [
]

xf_build.collect(srcs=srcs, inc_dirs=incs, requires=reqs, cflags=cflags)
