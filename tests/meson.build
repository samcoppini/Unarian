unarian_tests = [
    'add',
    'add_sub',
    'branches',
    'div',
    'factorial',
    'fibonacci',
    'mod',
    'mult',
    'nested',
    'sub',
]

python = import('python').find_installation('python3')

foreach test_name: unarian_tests
    cwd = meson.current_source_dir()

    test(
        test_name,
        python,
        args: [
            cwd / 'test_unarian.py',
            '--exe', unarian_exe,
            '--test', cwd / test_name + '.un',
        ]
    )
endforeach
