import argparse
import os
from collections import defaultdict
from typing import Dict, List, Set

SUMMARIES = "./summaries"
FAILURES = "./logs"


def get_additional_failures(file_name: str, failure_name: str, seen_failures: Set[str]):
    """ Search for build and testsuite failures """
    result = f"|{failure_name}|Additional Info|\n"
    result += "|---|---|\n"
    file_path = os.path.join(FAILURES, f"{file_name}")
    failures: Dict[str, Set[str]] = defaultdict(set)
    if os.path.exists(file_path) and os.path.getsize(file_path) > 0:
        with open(file_path, "r") as f:
            while True:
                line = f.readline().strip()
                if not line:
                    break
                artifact, comment = line.split("|")
                failures[artifact].add(comment)
        for failure, comment in failures.items():
            if failure not in seen_failures:
                result += f"|{failure}|{';'.join(comment)}|\n"
                seen_failures.add(failure)
        result += "\n"
        return result, seen_failures
    return "", seen_failures

def build_summary(failures: Dict[str, List[str]], failure_name: str):
    """ Builds table in summary section """
    tools = ("gcc", "g++", "gfortran")
    result = f"|{failure_name}|{tools[0]}|{tools[1]}|{tools[2]}|Previous Hash|\n"
    result += "|---|---|---|---|---|\n"
    result += f"{''.join(failures[failure_name.split(' ')[0]])}"
    result += "\n"
    return result

def failures_to_summary(failures: Dict[str, List[str]]):
    """ Builds summary section """
    result = "# Summary\n"
    seen_failures: Set[str] = set()
    additional_result, seen_failures = get_additional_failures("failed_build.txt", "Build Failures", seen_failures)
    result += additional_result
    additional_result, seen_failures = get_additional_failures("failed_testsuite.txt", "Testsuite Failures", seen_failures)
    result += additional_result

    result += build_summary(failures, "New Failures")
    result += build_summary(failures, "Resolved Failures")
    result += build_summary(failures, "Unresolved Failures")
    result += "\n"
    return result

def assign_labels(file_name: str, label: str):
    """Creates label for issue"""
    file_path = os.path.join(FAILURES, f"{file_name}")
    if os.path.exists(file_path) and os.path.getsize(file_path) > 0:
        return label
    return ""

def failures_to_markdown(failures: Dict[str, List[str]], current_hash: str):
    assignees = ["patrick-rivos", "kevinl-rivosinc", "ewlu"]
    result = f"""---
title: Testsuite Status {current_hash}
assignees: {", ".join(assignees)}
"""
    labels = {"bug"}
    labels.add(assign_labels("failed_build.txt", "build-failure"))
    labels.add(assign_labels("failed_testsuite.txt", "testsuite-failure"))
    if "" in labels:
        labels.remove("")
    result += f"labels: {', '.join(labels)}\n"
    result += "---\n\n"
    result += failures_to_summary(failures)
    return result

def aggregate_summary(failures: Dict[str, List[str]], file_name: str):
    """
    Reads file and adds the new failures to the current
    list of failures
    """
    with open(file_name, "r") as f:
        while True:
            line = f.readline()
            if not line:
                break
            if line.startswith("# Summary"):
                break
        while True:
            line = f.readline()
            if not line:
                break
            if "#" in line:
                # exited Summary section and going to New Failures section
                # TODO: add support for consolidating these sections
                break
            if "Failures" in line:
                index = line.split("Failures")[0][1:-1]
                continue
            if line != "\n" and "---" not in line:
                cells = line.split("|")
                if "linux" in file_name:
                    cells[1] = "linux: " + cells[1]
                else:
                    cells[1] = "newlib: " + cells[1]
                failures[index].append("|".join(cells))
    return failures

def parse_arguments():
    parser = argparse.ArgumentParser(description="Testsuite Compare Options")
    parser.add_argument(
        "-chash",
        "--current-hash",
        metavar="<string>",
        required=True,
        type=str,
        help="Commit hash of the current GCC testsuite log",
    )
    parser.add_argument(
        "-o",
        "--output-markdown",
        default="./testsuite.md",
        metavar="<filename>",
        type=str,
        help="Path to the current testsuite result log",
    )
    return parser.parse_args()


def main():
    args = parse_arguments()
    failures: Dict[str, List[str]] = { "Resolved": [], "Unresolved": [], "New": [] }
    for file in os.listdir(SUMMARIES):
        failures = aggregate_summary(failures, os.path.join(SUMMARIES, file))
    markdown = failures_to_markdown(failures, args.current_hash)
    with open(args.output_markdown, "w") as markdown_file:
        markdown_file.write(markdown)

if __name__ == "__main__":
    main()
