#!/usr/bin/env python3
from pathlib import Path
import argparse
from dataclasses import dataclass, field
from typing import List, Dict, Tuple, Set


@dataclass
class LibName:
    """Named Tuple for arch abi model"""

    arch: str
    abi: str
    model: str

    def __init__(self, arch: str, abi: str, model: str):
        self.arch = arch.strip().lower()
        self.abi = abi.strip().lower()
        self.model = model.strip().lower()

    def __str__(self):
        return " ".join((self.arch, self.abi, self.model))

    def __hash__(self):
        return hash((self.arch, self.abi, self.model))


@dataclass
class Description:
    """Named Tuple for tool arch abi model"""

    tool: str
    libname: LibName

    def __hash__(self):
        return hash((self.tool, self.libname))


@dataclass
class GccFailure:
    """Failure class to group lib's tool failures"""

    gcc_failure_count: Tuple[str, str] = ("0", "0")
    gpp_failure_count: Tuple[str, str] = ("0", "0")
    gfortran_failure_count: Tuple[str, str] = ("0", "0")

    gcc: Dict[str, Set[str]] = field(default_factory=dict)
    gpp: Dict[str, Set[str]] = field(default_factory=dict)
    gfortran: Dict[str, Set[str]] = field(default_factory=dict)

    def __str__(self):
        result = ""
        if len(self.gcc) > 0:
            result += "### gcc failures\n"
            for _, case in self.gcc.items():
                result += "\n".join(case)
        if len(self.gpp) > 0:
            result += "### g++ failures\n"
            for _, case in self.gpp.items():
                result += "\n".join(case)
        if len(self.gfortran) > 0:
            result += "### gfortran failures\n"
            for _, case in self.gfortran.items():
                result += "\n".join(case)
        return result

    def count_failures(
        self, unique_failure_dict: Dict[str, Set[str]]
    ) -> Tuple[str, str]:
        """parse (total failures count, unique failures count)"""
        unique_count = len(unique_failure_dict)
        total_count = 0
        for unique_failures in unique_failure_dict:
            total_count += len(unique_failure_dict[unique_failures])
        return (str(total_count), str(unique_count))

    def __setitem__(self, key, value: Dict[str, Set[str]]):
        if key == "gcc":
            self.gcc = value
            self.gcc_failure_count = self.count_failures(value)

        elif key == "g++":
            self.gpp = value
            self.gpp_failure_count = self.count_failures(value)

        elif key == "gfortran":
            self.gfortran = value
            self.gfortran_failure_count = self.count_failures(value)

    def __getitem__(self, key):
        if key == "gcc":
            return self.gcc
        elif key == "g++":
            return self.gpp
        elif key == "gfortran":
            return self.gfortran
        elif key == "gcc_failure_count":
            return self.gcc_failure_count
        elif key == "g++_failure_count":
            return self.gpp_failure_count
        elif key == "gfortran_failure_count":
            return self.gfortran_failure_count


@dataclass
class ClassifedGccFailures:
    """Failures class to distinguish the failure types"""

    resolved: Dict[LibName, GccFailure] = field(default_factory=dict)
    unresolved: Dict[LibName, GccFailure] = field(default_factory=dict)
    new: Dict[LibName, GccFailure] = field(default_factory=dict)

    def failure_dict_to_string(
        self, failure_dict: Dict[LibName, GccFailure], failure_name
    ):
        result = f"# {failure_name}\n"
        for libname, gccfailure in failure_dict.items():
            result += f"## {libname}\n"
            result += str(gccfailure)
        return result

    def __str__(self):
        result = self.failure_dict_to_string(self.resolved, "Resolved Failures")
        result += self.failure_dict_to_string(self.unresolved, "Unresolved Failures")
        result += self.failure_dict_to_string(self.new, "New Failures")
        return result


def parse_arguments():
    parser = argparse.ArgumentParser(description="Testsuite Compare Options")
    parser.add_argument(
        "-plog",
        "--previous-log",
        metavar="<filename>",
        required=True,
        type=str,
        help="Path to the previous testsuite result log",
    )
    parser.add_argument(
        "-phash",
        "--previous-hash",
        metavar="<string>",
        required=True,
        type=str,
        help="Commit hash of the previous GCC testsuite log",
    )

    parser.add_argument(
        "-clog",
        "--current-log",
        metavar="<filename>",
        required=True,
        type=str,
        help="Path to the current testsuite result log",
    )

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


def is_description(line: str) -> bool:
    """checks if the line is a tool description"""
    if line.startswith("\t\t==="):
        return True
    return False


def parse_description(line: str) -> Description:
    """returns 'tool arch abi model'"""
    descriptions = line.split(" ")
    tool = descriptions[1][:-1]
    arch = descriptions[5]
    abi = descriptions[6]
    model = descriptions[7]
    description = Description(tool, LibName(arch, abi, model))
    return description


def parse_failure_name(failure_line: str) -> str:
    failure_components = failure_line.split(" ")
    if len(failure_components) < 2:
        raise ValueError(f"Invalid Failure Log: {failure_line}")
    return failure_components[1]


def parse_testsuite_failures(log_path) -> Dict[Description, Set[str]]:
    """
    parse testsuite failures from the log in the path
    """
    if not Path(log_path).exists():
        raise ValueError(f"Invalid Path: {log_path}")
    failures = {}
    with open(log_path, "r") as file:
        description = None
        for line in file:
            if line == "\n":
                break
            if is_description(line):
                description = parse_description(line)
                failures[description] = set()
                continue
            failures[description].add(line)
    return failures


def classify_by_unique_failure(failure_set):
    failure_dictionary = {}
    for failure in failure_set:
        failure_name = parse_failure_name(failure)
        if failure_name not in failure_dictionary:
            failure_dictionary[failure_name] = set()
        failure_dictionary[failure_name].add(failure)
    return failure_dictionary


def compare_testsuite_log(previous_log_path: str, current_log_path: str):
    """
    returns (resolved_failures, unresolved_failures, new_failures)
    failures: Dict[tool combination label : Dict[unique testsuite name: Set[testsuite failure log]]]
    tool combination: 'tool arch abi model'
    """
    previous_failures = parse_testsuite_failures(previous_log_path)
    current_failures = parse_testsuite_failures(current_log_path)

    previous_failures_descriptions = set(previous_failures.keys())
    current_failures_descriptions = set(current_failures.keys())
    resolved_descriptions = (
        previous_failures_descriptions - current_failures_descriptions
    )
    unresolved_descriptions = (
        previous_failures_descriptions & current_failures_descriptions
    )
    new_descriptions = current_failures_descriptions - previous_failures_descriptions

    classified_gcc_failures = ClassifedGccFailures()
    for description in resolved_descriptions:
        classified_dict = classify_by_unique_failure(previous_failures[description])
        if len(classified_dict):
            classified_gcc_failures.resolved.setdefault(
                description.libname, GccFailure()
            )
            classified_gcc_failures.resolved[description.libname][
                description.tool
            ] = classified_dict

    for description in new_descriptions:
        classified_dict = classify_by_unique_failure(current_failures[description])
        if len(classified_dict):
            classified_gcc_failures.new.setdefault(description.libname, GccFailure())
            classified_gcc_failures.new[description.libname][
                description.tool
            ] = classified_dict

    for description in unresolved_descriptions:
        resolved_set = previous_failures[description] - current_failures[description]
        unresolved_set = previous_failures[description] & current_failures[description]
        new_set = current_failures[description] - previous_failures[description]
        classified_dict = classify_by_unique_failure(resolved_set)
        if len(classified_dict):
            classified_gcc_failures.resolved.setdefault(
                description.libname, GccFailure()
            )
            classified_gcc_failures.resolved[description.libname][
                description.tool
            ] = classified_dict

        classified_dict = classify_by_unique_failure(unresolved_set)
        if len(classified_dict):
            classified_gcc_failures.unresolved.setdefault(
                description.libname, GccFailure()
            )
            classified_gcc_failures.unresolved[description.libname][
                description.tool
            ] = classified_dict

        classified_dict = classify_by_unique_failure(new_set)
        if len(classified_dict):
            classified_gcc_failures.new.setdefault(description.libname, GccFailure())
            classified_gcc_failures.new[description.libname][
                description.tool
            ] = classified_dict

    return classified_gcc_failures


def gccfailure_to_summary(failure: Dict[LibName, GccFailure], failure_name: str, previous_hash: str, current_hash: str):
    tools = ("gcc", "g++", "gfortran")
    result = f"|{failure_name}|{tools[0]}|{tools[1]}|{tools[2]}|Previous Hash|\n"
    result +="|---|---|---|---|---|\n"
    for libname, gccfailure in failure.items():
        result += f"|{libname}|"
        for tool in tools:
            tool_failure_key = f"{tool}_failure_count"
            # convert tuple of counts to string
            result += f"{'/'.join(gccfailure[tool_failure_key])}|"
        result += f"[{previous_hash}](https://github.com/gcc-mirror/gcc/compare/{previous_hash}...{current_hash})|\n"
    result += "\n"
    return result


def failures_to_summary(failures: ClassifedGccFailures, previous_hash: str, current_hash: str):
    result = "# Summary\n"
    result += gccfailure_to_summary(failures.resolved, "Resolved Failures", previous_hash, current_hash)
    result += gccfailure_to_summary(failures.unresolved, "Unresolved Failures", previous_hash, current_hash)
    result += gccfailure_to_summary(failures.new, "New Failures", previous_hash, current_hash)
    result += "\n"
    return result


def failures_to_markdown(
    failures: ClassifedGccFailures, previous_hash, current_hash
) -> str:
    assignees = ("patrick-rivos", "kevinl-rivos")
    result = f"""---
title: {previous_hash}->{current_hash}
assignees: {str(assignees)}
labels: bug
---\n"""
    result += failures_to_summary(failures, previous_hash, current_hash)
    result += str(failures)
    return result


def is_result_valid(log_path: str):
    if not Path(log_path).exists():
        raise ValueError(f"Invalid Path: {log_path}")
    with open(log_path, "r") as file:
        summary_flag = False
        while True:
            line = file.readline()
            if not line:
                break
            if line.startswith(
                "               ========= Summary of gcc testsuite ========="
            ):
                summary_flag = True
                break
        if summary_flag == False:
            return False
        # Directly read the case line
        file.readline()
        file.readline()

        line = file.readline()
        # Remove Non-case elements
        splitted = line.split("|")[1:4]
        for case_number_str in splitted:
            case_number = case_number_str.split("/")
            # Test hasn't been executed for the tool
            if len(case_number) < 2:
                continue
            for case in case_number:
                if case.strip() == "":
                    return False
        return True

def compare_logs(previous_hash: str, previous_log: str, current_hash: str, current_log: str, output_markdown: str):
    if not is_result_valid(previous_log):
        raise RuntimeError(f"{previous_log} doesn't include Summary of the testsuite")
    if not is_result_valid(current_log):
        raise RuntimeError(f"{current_log} doesn't include Summary of the testsuite")
    failures = compare_testsuite_log(previous_log, current_log)
    markdown = failures_to_markdown(failures, previous_hash, current_hash)
    with open(output_markdown, "w") as markdown_file:
        markdown_file.write(markdown)


def main():
    args = parse_arguments()
    compare_logs(args.previous_hash, args.previous_log, args.current_hash, args.current_log, args.output_markdown)


if __name__ == "__main__":
    main()
