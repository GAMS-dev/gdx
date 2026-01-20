import os
from dataclasses import dataclass


@dataclass(frozen=True)
class OutputPaths:
    gdxdump: str
    gdxdiff: str
    gdxmerge: str


@dataclass(frozen=True)
class DirectoryPaths:
    gdx: str
    examples: str
    output: OutputPaths
    results: str


TESTS_DIRECTORY_PATH = os.path.dirname(os.path.abspath(__file__))
DIRECTORY_PATHS = DirectoryPaths(
    gdx=os.path.realpath(os.path.join(TESTS_DIRECTORY_PATH, "..", "..", "..")),
    examples=os.path.join(TESTS_DIRECTORY_PATH, "examples"),
    output=OutputPaths(
        gdxdump=os.path.join(TESTS_DIRECTORY_PATH, "output", "gdxdump"),
        gdxdiff=os.path.join(TESTS_DIRECTORY_PATH, "output", "gdxdiff"),
        gdxmerge=os.path.join(TESTS_DIRECTORY_PATH, "output", "gdxmerge"),
    ),
    results=os.path.join(TESTS_DIRECTORY_PATH, "results"),
)
