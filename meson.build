project(
    'unarian-cpp',
    'cpp',
    version: '0.1',
    default_options: [
        'cpp_std=c++20',
        'warning_level=3',
        'werror=true',
    ],
)

boost_dep = dependency('boost')

cli11_proj = subproject('cli11')
cli11_dep = cli11_proj.get_variable('CLI11_dep')

unarian_inc = include_directories('inc')

unarian_src = files(
    'src/bytecode.cpp',
    'src/main.cpp',
    'src/interpreter.cpp',
    'src/optimizer.cpp',
    'src/parser.cpp',
    'src/program.cpp',
    'src/token.cpp',
)

unarian_exe = executable(
    'unarian',
    unarian_src,
    include_directories: unarian_inc,
    dependencies: cli11_dep,
    install: true,
)
