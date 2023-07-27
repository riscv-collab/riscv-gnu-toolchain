import argparse
from github import Auth, Github
from download_artifacts import download_artifact


def parse_arguments():
    """ Parse command line arguments """
    parser = argparse.ArgumentParser(description="Download single log artifact")
    parser.add_argument(
        "-name",
        required=True,
        type=str,
        help="Name of the artifact",
    )
    parser.add_argument(
        "-token",
        required=True,
        type=str,
        help="Github access token",
    )
    parser.add_argument(
        "-outdir",
        required=True,
        type=str,
        help="output dir to put downloaded file"
    )
    return parser.parse_args()


def download_artifact_with_name(artifact_name: str, token: str, outdir: str):
    """
    Download the artifact with a given name into ./logs.
    """
    auth = Auth.Token(token)
    g = Github(auth=auth)

    repo = g.get_repo('patrick-rivos/riscv-gnu-toolchain')

    artifacts = repo.get_artifacts(artifact_name).get_page(0)
    if len(artifacts) != 0:
        download_artifact(artifact_name, str(artifacts[0].id), token, outdir)
    else:
        raise ValueError(f"Failed to find artifact for {artifact_name}")


def main():
    args = parse_arguments()
    download_artifact_with_name(args.name, args.token, args.outdir)


if __name__ == "__main__":
    main()
