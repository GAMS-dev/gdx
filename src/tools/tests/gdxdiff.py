import inspect
import os
import platform
import subprocess
import unittest

import gams.transfer as gt  # pyright: ignore[reportMissingTypeStubs]

from .common import DIRECTORY_PATHS
from .examples.default_values_examples import (
    create_default_values_example_1,
    create_default_values_example_2,
)
from .examples.description_examples import (
    create_description_example_1,
    create_description_example_2,
)
from .examples.domain_examples import create_domain_example_1, create_domain_example_2
from .examples.full_example import create_full_example
from .examples.full_example_changed_data_and_variables import (
    create_full_example_changed_data_and_variables,
)
from .examples.full_example_changed_variables import (
    create_full_example_changed_variables,
)
from .examples.order_examples import create_order_example_1, create_order_example_2
from .examples.small_example import create_small_example
from .examples.small_example_changed_data import create_small_example_changed_data


class TestGdxDiff(unittest.TestCase):
    FILE_NAMES = [
        "small_example",
        "full_example",
        "small_example_changed_data",
        "full_example_changed_variables",
        "full_example_changed_data_and_variables",
        "default_values_example_1",
        "default_values_example_2",
        "domain_example_1",
        "domain_example_2",
        "order_example_1",
        "order_example_2",
        "description_example_1",
        "description_example_2",
        "diff_file",
    ]
    FILE_PATHS: dict[str, str]

    @classmethod
    def setUpClass(cls) -> None:
        cls.FILE_PATHS = {
            file_name: os.path.join(DIRECTORY_PATHS.examples, f"{file_name}.gdx")
            for file_name in cls.FILE_NAMES
        }

        create_small_example(cls.FILE_PATHS["small_example"])
        create_full_example(cls.FILE_PATHS["full_example"])
        create_small_example_changed_data(cls.FILE_PATHS["small_example_changed_data"])
        create_full_example_changed_variables(
            cls.FILE_PATHS["full_example_changed_variables"]
        )
        create_full_example_changed_data_and_variables(
            cls.FILE_PATHS["full_example_changed_data_and_variables"]
        )
        create_default_values_example_1(cls.FILE_PATHS["default_values_example_1"])
        create_default_values_example_2(cls.FILE_PATHS["default_values_example_2"])
        create_domain_example_1(cls.FILE_PATHS["domain_example_1"])
        create_domain_example_2(cls.FILE_PATHS["domain_example_2"])
        create_order_example_1(cls.FILE_PATHS["order_example_1"])
        create_order_example_2(cls.FILE_PATHS["order_example_2"])
        create_description_example_1(cls.FILE_PATHS["description_example_1"])
        create_description_example_2(cls.FILE_PATHS["description_example_2"])

    @classmethod
    def tearDownClass(cls) -> None:
        for file_path in cls.FILE_PATHS.values():
            os.remove(file_path)

    @classmethod
    def run_gdxdiff(cls, command: list[str]) -> subprocess.CompletedProcess[str]:
        EXECUTABLE_NAME = "gdxdiff"
        executable_path: list[str]
        if platform.system() == "Windows":
            executable_path = (
                ["Release", f"{EXECUTABLE_NAME}.exe"]
                if os.path.isdir("Release")
                else ["gdxtools", f"{EXECUTABLE_NAME}.exe"]
            )
        else:
            build_directory_exists = os.path.isdir("build")
            os.environ[
                "LD_LIBRARY_PATH"
                if platform.system() == "Linux"
                else "DYLD_LIBRARY_PATH"
            ] = (
                os.path.join(DIRECTORY_PATHS.gdx, "build")
                if build_directory_exists
                else DIRECTORY_PATHS.gdx
            )
            executable_path = (
                ["build", "src", "tools", EXECUTABLE_NAME, EXECUTABLE_NAME]
                if build_directory_exists
                else ["gdxtools", EXECUTABLE_NAME]
            )
        return subprocess.run(
            [os.path.join(DIRECTORY_PATHS.gdx, *executable_path), *command],
            capture_output=True,
            text=True,
        )

    def check_output(
        self,
        output: subprocess.CompletedProcess[str],
        return_code: int = 0,
        file_name: str | None = None,
        first_offset: int | None = None,
        first_negative_offset: int | None = None,
        second_offset: int | None = None,
        second_negative_offset: int | None = None,
        first_delete: list[int] = [],
        second_delete: list[int] = [],
    ) -> None:
        self.assertEqual(output.returncode, return_code)
        first = output.stdout.split("\n")[first_offset:first_negative_offset]
        for i in first_delete:
            del first[i]
        if file_name is None:
            file_name = f"{inspect.stack()[1].function.removeprefix('test_')}.txt"
        with open(os.path.join(DIRECTORY_PATHS.output.gdxdiff, file_name), "r") as file:
            second = file.read().split("\n")[second_offset:second_negative_offset]
        for i in second_delete:
            del second[i]
        self.assertEqual(first, second)
        self.assertEqual(output.stderr, "")

    def check_gdx_file_symbols(
        self, container: gt.Container, symbol_names: list[str]
    ) -> None:
        for symbol_name in symbol_names:
            with self.subTest(symbol_name=symbol_name):
                self.assertIn(symbol_name, container)
        self.assertEqual(len(container), len(symbol_names))

    def check_gdx_file_values(
        self,
        container: gt.Container,
        symbol_name: str,
        expected_values: list[list[str | float]],
    ) -> None:
        self.assertIn(symbol_name, container)
        symbol: gt.Parameter = container[symbol_name]  # type: ignore
        values = symbol.records.values.tolist()  # type: ignore
        self.assertEqual(values, expected_values)

    def check_gdx_file(self, symbols: dict[str, list[list[str | float]]]) -> None:
        container = gt.Container(
            system_directory=os.environ.get("GAMS_SYSTEM_DIRECTORY"),
            load_from=self.FILE_PATHS["diff_file"],
        )
        self.check_gdx_file_symbols(container, list(symbols.keys()))
        for symbol_name in symbols:
            self.check_gdx_file_values(container, symbol_name, symbols[symbol_name])

    def test_empty_command(self) -> None:
        output = self.run_gdxdiff([])
        self.check_output(
            output,
            return_code=2,
            file_name="usage.txt",
            second_offset=1,
            first_delete=[1],
            second_delete=[1],
        )

    def test_small_example_and_full_example(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["small_example"],
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["diff_file"],
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "FilesCompared": [
                ["File1", self.FILE_PATHS["small_example"]],
                ["File2", self.FILE_PATHS["full_example"]],
            ]
        }
        self.check_gdx_file(symbols)

    def test_small_example_and_small_example_changed_data(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["small_example"],
                self.FILE_PATHS["small_example_changed_data"],
                self.FILE_PATHS["diff_file"],
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "d": [
                ["seattle", "new-york", "dif1", 2.5],
                ["seattle", "new-york", "dif2", 3.5],
                ["seattle", "chicago", "dif1", 1.7],
                ["seattle", "chicago", "dif2", 2.7],
                ["seattle", "topeka", "dif1", 1.8],
                ["seattle", "topeka", "dif2", 2.8],
                ["san-diego", "new-york", "dif1", 2.5],
                ["san-diego", "new-york", "dif2", 3.5],
                ["san-diego", "chicago", "dif1", 1.8],
                ["san-diego", "chicago", "dif2", 2.8],
                ["san-diego", "topeka", "dif1", 1.4],
                ["san-diego", "topeka", "dif2", 2.4],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["small_example"]],
                ["File2", self.FILE_PATHS["small_example_changed_data"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_small_example_and_small_example_changed_data_epsilon_absolute_1(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["small_example"],
                self.FILE_PATHS["small_example_changed_data"],
                self.FILE_PATHS["diff_file"],
                "Eps=1",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            file_name="small_example_and_small_example_changed_data.txt",
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "d": [
                ["seattle", "chicago", "dif1", 1.7],
                ["seattle", "chicago", "dif2", 2.7],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["small_example"]],
                ["File2", self.FILE_PATHS["small_example_changed_data"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_small_example_and_small_example_changed_data_epsilon_absolute_2(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["small_example"],
                self.FILE_PATHS["small_example_changed_data"],
                self.FILE_PATHS["diff_file"],
                "Eps=2",
            ]
        )
        self.check_output(
            output,
            return_code=0,
            file_name="small_example_and_small_example_changed_data_epsilon.txt",
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "FilesCompared": [
                ["File1", self.FILE_PATHS["small_example"]],
                ["File2", self.FILE_PATHS["small_example_changed_data"]],
            ]
        }
        self.check_gdx_file(symbols)

    def test_small_example_and_small_example_changed_data_epsilon_relative(
        self,
    ) -> None:
        for i in [1, 2]:
            with self.subTest(i=i):
                output = self.run_gdxdiff(
                    [
                        self.FILE_PATHS["small_example"],
                        self.FILE_PATHS["small_example_changed_data"],
                        self.FILE_PATHS["diff_file"],
                        f"RelEps={i}",
                    ]
                )
                self.check_output(
                    output,
                    return_code=0,
                    file_name="small_example_and_small_example_changed_data_epsilon.txt",
                    first_offset=3,
                    second_offset=3,
                    first_delete=[-3],
                    second_delete=[-3],
                )

                symbols: dict[str, list[list[str | float]]] = {
                    "FilesCompared": [
                        ["File1", self.FILE_PATHS["small_example"]],
                        ["File2", self.FILE_PATHS["small_example_changed_data"]],
                    ]
                }
                self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_variables"],
                self.FILE_PATHS["diff_file"],
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "x": [
                ["seattle", "new-york", "dif1", 50.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "new-york", "dif2", 150.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif1", 300.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif2", 400.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_all(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_variables"],
                self.FILE_PATHS["diff_file"],
                "Field=All",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            file_name="full_example_and_full_example_changed_variables.txt",
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "x": [
                ["seattle", "new-york", "dif1", 50.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "new-york", "dif2", 150.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif1", 300.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif2", 400.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_l(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_variables"],
                self.FILE_PATHS["diff_file"],
                "Field=L",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            file_name="full_example_and_full_example_changed_variables.txt",
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "x": [
                ["seattle", "new-york", "dif1", 50.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "new-york", "dif2", 150.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif1", 300.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif2", 400.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_m(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_variables"],
                self.FILE_PATHS["diff_file"],
                "Field=M",
            ]
        )
        self.check_output(
            output,
            return_code=0,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_variables"]],
            ]
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_l_field_only(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_variables"],
                self.FILE_PATHS["diff_file"],
                "Field=L",
                "FldOnly",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            file_name="full_example_and_full_example_changed_variables.txt",
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "x": [
                ["seattle", "new-york", "dif1", 50.0],
                ["seattle", "new-york", "dif2", 150.0],
                ["seattle", "chicago", "dif1", 300.0],
                ["seattle", "chicago", "dif2", 400.0],
                ["san-diego", "new-york", "dif1", 275.0],
                ["san-diego", "new-york", "dif2", 375.0],
                ["san-diego", "topeka", "dif1", 275.0],
                ["san-diego", "topeka", "dif2", 375.0],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_field_m_field_only(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_variables"],
                self.FILE_PATHS["diff_file"],
                "Field=M",
                "FldOnly",
            ]
        )
        self.check_output(
            output,
            return_code=0,
            file_name="full_example_and_full_example_changed_variables_field_m.txt",
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_variables"]],
            ]
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_data_and_variables(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_data_and_variables"],
                self.FILE_PATHS["diff_file"],
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "c": [
                ["seattle", "new-york", "dif1", 0.225],
                ["seattle", "new-york", "dif2", 0.315],
                ["seattle", "chicago", "dif1", 0.153],
                ["seattle", "chicago", "dif2", 0.24300000000000002],
                ["seattle", "topeka", "dif1", 0.162],
                ["seattle", "topeka", "dif2", 0.25199999999999995],
                ["san-diego", "new-york", "dif1", 0.225],
                ["san-diego", "new-york", "dif2", 0.315],
                ["san-diego", "chicago", "dif1", 0.162],
                ["san-diego", "chicago", "dif2", 0.25199999999999995],
                ["san-diego", "topeka", "dif1", 0.12599999999999997],
                ["san-diego", "topeka", "dif2", 0.216],
            ],
            "d": [
                ["seattle", "new-york", "dif1", 2.5],
                ["seattle", "new-york", "dif2", 3.5],
                ["seattle", "chicago", "dif1", 1.7],
                ["seattle", "chicago", "dif2", 2.7],
                ["seattle", "topeka", "dif1", 1.8],
                ["seattle", "topeka", "dif2", 2.8],
                ["san-diego", "new-york", "dif1", 2.5],
                ["san-diego", "new-york", "dif2", 3.5],
                ["san-diego", "chicago", "dif1", 1.8],
                ["san-diego", "chicago", "dif2", 2.8],
                ["san-diego", "topeka", "dif1", 1.4],
                ["san-diego", "topeka", "dif2", 2.4],
            ],
            "x": [
                ["seattle", "new-york", "dif1", 50.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "new-york", "dif2", 150.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif1", 300.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif2", 400.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_data_and_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_data_and_variables_id_x(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_data_and_variables"],
                self.FILE_PATHS["diff_file"],
                "ID=x",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "x": [
                ["seattle", "new-york", "dif1", 50.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "new-york", "dif2", 150.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif1", 300.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif2", 400.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_data_and_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

        output_space_separator = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_data_and_variables"],
                self.FILE_PATHS["diff_file"],
                "ID",
                "x",
            ]
        )
        self.check_output(
            output_space_separator,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_data_and_variables_id_c_d(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_data_and_variables"],
                self.FILE_PATHS["diff_file"],
                "ID=c",
                "ID=d",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "c": [
                ["seattle", "new-york", "dif1", 0.225],
                ["seattle", "new-york", "dif2", 0.315],
                ["seattle", "chicago", "dif1", 0.153],
                ["seattle", "chicago", "dif2", 0.24300000000000002],
                ["seattle", "topeka", "dif1", 0.162],
                ["seattle", "topeka", "dif2", 0.25199999999999995],
                ["san-diego", "new-york", "dif1", 0.225],
                ["san-diego", "new-york", "dif2", 0.315],
                ["san-diego", "chicago", "dif1", 0.162],
                ["san-diego", "chicago", "dif2", 0.25199999999999995],
                ["san-diego", "topeka", "dif1", 0.12599999999999997],
                ["san-diego", "topeka", "dif2", 0.216],
            ],
            "d": [
                ["seattle", "new-york", "dif1", 2.5],
                ["seattle", "new-york", "dif2", 3.5],
                ["seattle", "chicago", "dif1", 1.7],
                ["seattle", "chicago", "dif2", 2.7],
                ["seattle", "topeka", "dif1", 1.8],
                ["seattle", "topeka", "dif2", 2.8],
                ["san-diego", "new-york", "dif1", 2.5],
                ["san-diego", "new-york", "dif2", 3.5],
                ["san-diego", "chicago", "dif1", 1.8],
                ["san-diego", "chicago", "dif2", 2.8],
                ["san-diego", "topeka", "dif1", 1.4],
                ["san-diego", "topeka", "dif2", 2.4],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_data_and_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

        output_quotation_marks = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_data_and_variables"],
                self.FILE_PATHS["diff_file"],
                "ID='c d'",
            ]
        )
        self.check_output(
            output_quotation_marks,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_data_and_variables_skip_id_x(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_data_and_variables"],
                self.FILE_PATHS["diff_file"],
                "SkipID=x",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "c": [
                ["seattle", "new-york", "dif1", 0.225],
                ["seattle", "new-york", "dif2", 0.315],
                ["seattle", "chicago", "dif1", 0.153],
                ["seattle", "chicago", "dif2", 0.24300000000000002],
                ["seattle", "topeka", "dif1", 0.162],
                ["seattle", "topeka", "dif2", 0.25199999999999995],
                ["san-diego", "new-york", "dif1", 0.225],
                ["san-diego", "new-york", "dif2", 0.315],
                ["san-diego", "chicago", "dif1", 0.162],
                ["san-diego", "chicago", "dif2", 0.25199999999999995],
                ["san-diego", "topeka", "dif1", 0.12599999999999997],
                ["san-diego", "topeka", "dif2", 0.216],
            ],
            "d": [
                ["seattle", "new-york", "dif1", 2.5],
                ["seattle", "new-york", "dif2", 3.5],
                ["seattle", "chicago", "dif1", 1.7],
                ["seattle", "chicago", "dif2", 2.7],
                ["seattle", "topeka", "dif1", 1.8],
                ["seattle", "topeka", "dif2", 2.8],
                ["san-diego", "new-york", "dif1", 2.5],
                ["san-diego", "new-york", "dif2", 3.5],
                ["san-diego", "chicago", "dif1", 1.8],
                ["san-diego", "chicago", "dif2", 2.8],
                ["san-diego", "topeka", "dif1", 1.4],
                ["san-diego", "topeka", "dif2", 2.4],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_data_and_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

        output_space_separator = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_data_and_variables"],
                self.FILE_PATHS["diff_file"],
                "SkipID",
                "x",
            ]
        )
        self.check_output(
            output_space_separator,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_data_and_variables_skip_id_c_d(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_data_and_variables"],
                self.FILE_PATHS["diff_file"],
                "SkipID=c",
                "SkipID=d",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "x": [
                ["seattle", "new-york", "dif1", 50.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "new-york", "dif2", 150.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif1", 300.0, 0.0, 0.0, float("inf"), 1.0],
                ["seattle", "chicago", "dif2", 400.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "new-york", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif1", 275.0, 0.0, 0.0, float("inf"), 1.0],
                ["san-diego", "topeka", "dif2", 375.0, 0.0, 0.0, float("inf"), 1.0],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_data_and_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

        output_quotation_marks = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_data_and_variables"],
                self.FILE_PATHS["diff_file"],
                "SkipID='c d'",
            ]
        )
        self.check_output(
            output_quotation_marks,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )
        self.check_gdx_file(symbols)

    def test_full_example_and_full_example_changed_variables_differences_only(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["full_example"],
                self.FILE_PATHS["full_example_changed_variables"],
                self.FILE_PATHS["diff_file"],
                "DiffOnly",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            file_name="full_example_and_full_example_changed_variables.txt",
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "x": [
                ["seattle", "new-york", "Level", "dif1", 50.0],
                ["seattle", "new-york", "Level", "dif2", 150.0],
                ["seattle", "chicago", "Level", "dif1", 300.0],
                ["seattle", "chicago", "Level", "dif2", 400.0],
                ["san-diego", "new-york", "Level", "dif1", 275.0],
                ["san-diego", "new-york", "Level", "dif2", 375.0],
                ["san-diego", "topeka", "Level", "dif1", 275.0],
                ["san-diego", "topeka", "Level", "dif2", 375.0],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["full_example"]],
                ["File2", self.FILE_PATHS["full_example_changed_variables"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_default_values_example_1_and_default_values_example_2(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["default_values_example_1"],
                self.FILE_PATHS["default_values_example_2"],
                self.FILE_PATHS["diff_file"],
            ]
        )
        self.check_output(
            output,
            return_code=0,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "FilesCompared": [
                ["File1", self.FILE_PATHS["default_values_example_1"]],
                ["File2", self.FILE_PATHS["default_values_example_2"]],
            ]
        }
        self.check_gdx_file(symbols)

    def test_default_values_example_1_and_default_values_example_2_compare_default_values(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["default_values_example_1"],
                self.FILE_PATHS["default_values_example_2"],
                self.FILE_PATHS["diff_file"],
                "CmpDefaults",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "v": [["i0", "ins1", 0.0, 0.0, float("-inf"), float("inf"), 1.0]],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["default_values_example_1"]],
                ["File2", self.FILE_PATHS["default_values_example_2"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_domain_example_1_and_domain_example_2(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["domain_example_1"],
                self.FILE_PATHS["domain_example_2"],
                self.FILE_PATHS["diff_file"],
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "FilesCompared": [
                ["File1", self.FILE_PATHS["domain_example_1"]],
                ["File2", self.FILE_PATHS["domain_example_2"]],
            ]
        }
        self.check_gdx_file(symbols)

    def test_domain_example_1_and_domain_example_2_compare_symbol_domains(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["domain_example_1"],
                self.FILE_PATHS["domain_example_2"],
                self.FILE_PATHS["diff_file"],
                "CmpDomains",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "FilesCompared": [
                ["File1", self.FILE_PATHS["domain_example_1"]],
                ["File2", self.FILE_PATHS["domain_example_2"]],
            ]
        }
        self.check_gdx_file(symbols)

    def test_order_example_1_and_order_example_2(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["order_example_1"],
                self.FILE_PATHS["order_example_2"],
                self.FILE_PATHS["diff_file"],
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "t1": [
                ["1988", "ins1", ""],
                ["1990", "ins1", ""],
                ["1983", "ins2", ""],
                ["1985", "ins2", ""],
            ],
            "t2": [
                ["1988", "ins2", ""],
                ["1989", "ins2", ""],
                ["1990", "ins2", ""],
                ["1991", "ins2", ""],
                ["1983", "ins1", ""],
                ["1984", "ins1", ""],
                ["1985", "ins1", ""],
                ["1986", "ins1", ""],
            ],
            "t3": [
                ["1989", "ins1", ""],
                ["1991", "ins1", ""],
                ["1984", "ins2", ""],
                ["1986", "ins2", ""],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["order_example_1"]],
                ["File2", self.FILE_PATHS["order_example_2"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_order_example_1_and_order_example_2_ignore_order(self) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["order_example_1"],
                self.FILE_PATHS["order_example_2"],
                self.FILE_PATHS["diff_file"],
                "IgnoreOrder",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            file_name="order_example_1_and_order_example_2.txt",
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "t1": [
                ["1988", "ins1", ""],
                ["1990", "ins1", ""],
                ["1983", "ins2", ""],
                ["1985", "ins2", ""],
            ],
            "t2": [
                ["1988", "ins2", ""],
                ["1990", "ins2", ""],
                ["1983", "ins1", ""],
                ["1985", "ins1", ""],
                ["1989", "ins2", ""],
                ["1991", "ins2", ""],
                ["1984", "ins1", ""],
                ["1986", "ins1", ""],
            ],
            "t3": [
                ["1989", "ins1", ""],
                ["1991", "ins1", ""],
                ["1984", "ins2", ""],
                ["1986", "ins2", ""],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["order_example_1"]],
                ["File2", self.FILE_PATHS["order_example_2"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_description_example_1_and_description_example_2_compare_associated_texts_y(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["description_example_1"],
                self.FILE_PATHS["description_example_2"],
                self.FILE_PATHS["diff_file"],
                "SetDesc=Y",
            ]
        )
        self.check_output(
            output,
            return_code=1,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "i": [
                ["seattle", "dif1", "text 1"],
                ["seattle", "dif2", "text 3"],
                ["san-diego", "dif1", "text 2"],
                ["san-diego", "dif2", "text 4"],
            ],
            "FilesCompared": [
                ["File1", self.FILE_PATHS["description_example_1"]],
                ["File2", self.FILE_PATHS["description_example_2"]],
            ],
        }
        self.check_gdx_file(symbols)

    def test_description_example_1_and_description_example_2_compare_associated_texts_n(
        self,
    ) -> None:
        output = self.run_gdxdiff(
            [
                self.FILE_PATHS["description_example_1"],
                self.FILE_PATHS["description_example_2"],
                self.FILE_PATHS["diff_file"],
                "SetDesc=N",
            ]
        )
        self.check_output(
            output,
            return_code=0,
            first_offset=3,
            second_offset=3,
            first_delete=[-3],
            second_delete=[-3],
        )

        symbols: dict[str, list[list[str | float]]] = {
            "FilesCompared": [
                ["File1", self.FILE_PATHS["description_example_1"]],
                ["File2", self.FILE_PATHS["description_example_2"]],
            ]
        }
        self.check_gdx_file(symbols)

    # TODO: Test the MatrixFile option as well
