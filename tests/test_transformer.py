import subprocess
import re
from pathlib import Path

ARGS_RE = r'ARGS: (.*)'
timeout = 5


def run_pipe(cmds, input, timeout):
    """Execute a pipeline of shell commands.

    Send the given input (text) string into the first command, then pipe
    the output of each command into the next command in the sequence.
    Collect and return the stdout and stderr from the final command.
    """
    procs = []
    for cmd in cmds:
        last = len(procs) == len(cmds) - 1
        proc = subprocess.Popen(
            cmd,
            shell=True,
            text=True,
            stdin=procs[-1].stdout if procs else subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE if last else subprocess.DEVNULL,
        )
        procs.append(proc)

    try:
        # Send stdin and collect stdout.
        procs[0].stdin.write(input)
        procs[0].stdin.close()
        return procs[-1].communicate(timeout=timeout)
    finally:
        for proc in procs:
            proc.kill()


def run_test(pipeline, fn, timeout):
    """Run a single test pipeline.
    """
    # Load the benchmark.
    with open(fn) as f:
        in_data = f.read()

    # Extract arguments.
    match = re.search(ARGS_RE, in_data)
    args = match.group(1) if match else ''

    # Run pipeline.
    cmds = [
        c.format(args=args)
        for c in pipeline
    ]
    return run_pipe(cmds, in_data, timeout)


def run_all(pipeline, files):
    def convert(val):
        if val.lower() == "true":
            return True
        elif val.lower() == "false":
            return False

        try:
            return float(val)
        except ValueError:
            return val

    results = {}
    for f in files:
        try:
            out, err = run_test(pipeline, f, timeout)
            results[f] = [convert(r) for r in out.splitlines() if r.strip()]
        except subprocess.TimeoutExpired:
            results[f] = 'fail'
    return results


def baseline(files):
    pipeline = [
        "bril2json",
        "brilirs {args}",
    ]
    return run_all(pipeline, files)


def transformer(files, expected):
    pipeline = [
        "bril2json",
        "../build/sc",
        "bril2json",
        "brilirs {args}",
    ]
    result = run_all(pipeline, files)
    for f in files:
        name = f.split('/')[-1]
        got = result[f]
        exp = expected[f]
        if got == exp:
            print("Pass: {}".format(name))
        else:
            print("Fail: {}. Expected: {} but found: {}".format(name, exp, got))


def main():
    # path = Path("/home/pal/workspace/external-projects/bril/benchmarks/core/")
    # path = Path("/home/pal/workspace/external-projects/bril/benchmarks/float/")
    path = Path("/home/pal/workspace/external-projects/bril/benchmarks/mem/")
    # path = Path("bril/transformer/")
    files = [str(f.resolve()) for f in path.glob("*.bril")]
    results = baseline(files)
    transformer(files, results)


if __name__ == '__main__':
    main()
