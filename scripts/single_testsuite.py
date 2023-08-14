#!/usr/bin/env python3

import os
import subprocess
import logging
import argparse
from pathlib import Path


def parse_arguments(parser: argparse.ArgumentParser):
    parser.add_argument(
        "-tt",
        "--target-test",
        metavar="<string>",
        required=True,
        type=str,
        help=(
            "List the target test. Expected form is $testexp=$testname"
            " ex)riscv.exp=arch-1.c"
        ),
    )

    parser.add_argument(
        "-bdir",
        "--build-directory",
        metavar="<directory>",
        default="./",
        type=str,
        help="The stage2 build directory where the testsuite command would be executed",
    )

    parser.add_argument(
        "-tb",
        "--target-board",
        default="rv64gc-lp64d",
        metavar="<string>",
        type=str,
        nargs="*",
        help=(
            "List of the target boards. The format is $arch-$abi $arch-$abi ... use - to"
            " seaparate arch and abi ex)--target-board rv64gc-lp64d rv64ima-ilp32"
            " rv64g-lp64d"
        ),
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Testsuite execution verbose level",
    )

    parser.add_argument(
        "-vv",
        "--very-verbose",
        action="store_true",
        help="Testsuite execution extra verbose level",
    )
    return parser.parse_args()


def parse_target_board(target_board_input: list):
    result = ""
    for target_board in target_board_input:
        parsed_target_board = target_board.strip()
        parsed_target_board = target_board.split("-")
        if len(parsed_target_board) != 2:
            raise ValueError(f"Invalid target board format: {target_board}")
        result += (
            f"riscv-sim/-march={parsed_target_board[0]}/-mabi={parsed_target_board[1]}/-mcmodel=medlow "
        )
    return result


def parse_directories(args):
    """Returns tuple (root directory, install directory). Returns None if it doesn't exist"""
    build_directory = Path(args.build_directory)
    if not build_directory.exists():
        raise ValueError(f"Invalid Directory: {build_directory}")
    root_directory = None
    install_directory = None
    makefile_directory = build_directory / "Makefile"
    with open(makefile_directory, "r") as makefile:
        for line in makefile:
            if line.startswith("srcdir ="):
                _, source_string = line.split("=", 1)
                source_string = source_string.strip()
                source_directory = Path(source_string)
                if not source_directory.is_absolute():
                    source_directory = build_directory / source_string
                root_directory = (source_directory / "../").resolve()

            if line.startswith("prefix ="):
                _, prefix_string = line.split("=", 1)
                prefix_string = prefix_string.strip()
                prefix_directory = Path(prefix_string)
                if not prefix_directory.is_absolute():
                    prefix_directory = build_directory / prefix_string
                install_directory = prefix_directory.resolve()

            if root_directory != None and install_directory != None:
                return root_directory, install_directory
    return None, None


def setup_environment(args):
    current_environment = os.environ.copy()
    current_environment[
        "PATH"
    ] = f'{args.root_directory}/scripts/wrapper/qemu:{args.install_directory}/bin:{current_environment["PATH"]}'

    prerequisite_command = (
        f"make stamps/build-qemu stamps/build-dejagnu -C {args.root_directory}"
    )
    prerequisite_process = subprocess.Popen(
        prerequisite_command,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=current_environment,
    )
    prerequisite_result, prerequisite_error = prerequisite_process.communicate()
    prerequisite_process.kill()

    return current_environment


def main():
    parser = argparse.ArgumentParser(description="Testsuite Options")
    args = parse_arguments(parser)

    root_directory, install_directory = parse_directories(args)
    if root_directory == None or install_directory == None:
        raise RuntimeError(
            "Makefile is missing directory paths. Please specify appropriate build path"
        )
    args.root_directory = root_directory
    args.install_directory = install_directory
    current_environment = setup_environment(args)
    target_board_option = parse_target_board(args.target_board)

    verbose_option = ""
    if args.verbose:
        verbose_option = " -v"
    elif args.very_verbose:
        verbose_option = " -v -v"

    command = (
        "make check-gcc -C"
        f" {args.build_directory} \"RUNTESTFLAGS=--target_board='{target_board_option}'"
        f' {verbose_option} {args.target_test}"'
    )

    print("\nEXECUTION PATH:\n", current_environment["PATH"])
    print("\nEXECUTION COMMAND:\n", command)

    testsuite_process = subprocess.Popen(
        command,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=current_environment,
    )
    testsuite_result, testsuite_error = testsuite_process.communicate()

    if testsuite_error:
        print("\nTESTSUITE EXECUTION ERROR: \n", testsuite_error.decode())
    print("\nTESTSUITE EXECUTION RESULT: \n", testsuite_result.decode())

    testsuite_process.kill()


if __name__ == "__main__":
    main()
