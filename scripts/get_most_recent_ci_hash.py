# Gets the most recent hash that has an associated CI run

import os
import argparse
from typing import List
from github import Auth, Github

def gcc_hashes(hash: str, subsequent: bool):
    """ Get the most recent GCC hashes within a 100 commits (in order from closest to furthest)"""
    if subsequent is False: # Get prior commit
        old_commit = os.popen(f'cd gcc && git checkout master --quiet && git pull --quiet && git rev-parse {hash}~100').read().strip()
        print(f'git rev-list --format=short --ancestry-path {old_commit}~1..{hash}~1')
        commits = os.popen(f'cd gcc && git rev-list --ancestry-path {old_commit}^..{hash}^').read()
        commits = commits.splitlines()
    else:
        os.popen('cd gcc && git checkout master --quiet && git pull --quiet').read()
        commits = os.popen(f'cd gcc && git rev-list --ancestry-path {hash}..HEAD | head -100').read()
        commits = list(reversed(commits.splitlines()))

    return commits

def get_valid_artifact_hash(hashes:List[str], token: str, artifact_name: str):
    """ 
    Searches for the most recent GCC hash that has the artifact specified by
    @param artifact_name. Also returns id of found artifact for download
    """
    auth = Auth.Token(token)
    g = Github(auth=auth)

    repo = g.get_repo('patrick-rivos/riscv-gnu-toolchain')

    for hash in hashes:
        artifacts = repo.get_artifacts(artifact_name.format(hash)).get_page(0)
        if len(artifacts) != 0:
            return hash, artifacts[0].id

    return "No valid hash", -1
  
def main(hash: str, subsequent: bool, token: str):
    commits = gcc_hashes(hash, subsequent)
    print(get_valid_hash(commits, token))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Get closest valid GCC hash")
    parser.add_argument(
        "-hash",
        required=True,
        type=str,
        help="Commit hash of GCC to get artifacts for",
    )
    parser.add_argument(
        "-subsequent",
        action='store_true',
        help="Get the subsequent closest commit. Defaults to prior.",
    )
    parser.add_argument(
        "-token",
        required=True,
        type=str,
        help="Github access token.",
    )
    args = parser.parse_args()

    main(args.hash, args.subsequent, args.token)
