unarian_tests = [
    'add_sub',
    'branches',
    'decrement',
    'div',
    'div_if',
    'double',
    'factorial',
    'fibonacci',
    'increment',
    'million',
    'nested',
    'not',
    'zero',
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
