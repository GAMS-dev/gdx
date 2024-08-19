import sys
import argparse
import unittest
import gams
from gdxdump import TestGdxDump
from gdxdiff import TestGdxDiff
from gdxmerge import TestGdxMerge


class TestGdxTools(unittest.TestCase):

    def test_gams_module(self) -> None:
        self.assertTrue('gams' in sys.modules)

    def test_gams_api(self) -> None:
        self.assertRegex(
            f'API OK -- Version {gams.__version__}',
            r'API OK -- Version \d+\.\d+\.\d+'
        )


def main() -> int:
    parser = argparse.ArgumentParser(description='gdxtools tests')
    parser.add_argument('-t', '--tool', help='specify the tool you want to test')
    args = parser.parse_args()

    def suite() -> unittest.TestSuite:
        suite = unittest.TestSuite()
        loader = unittest.TestLoader()

        suite.addTests(loader.loadTestsFromTestCase(TestGdxTools))

        match args.tool:
            case 'gdxdump' | 'dump':
                suite.addTests(loader.loadTestsFromTestCase(TestGdxDump))

            case 'gdxdiff' | 'diff':
                suite.addTests(loader.loadTestsFromTestCase(TestGdxDiff))

            case 'gdxmerge' | 'merge':
                suite.addTests(loader.loadTestsFromTestCase(TestGdxMerge))

            case _:
                suite.addTests(loader.loadTestsFromTestCase(TestGdxDump))
                suite.addTests(loader.loadTestsFromTestCase(TestGdxDiff))
                suite.addTests(loader.loadTestsFromTestCase(TestGdxMerge))

        return suite

    runner = unittest.TextTestRunner()
    result = runner.run(suite())

    if result.wasSuccessful():
        return 0
    else:
        return 1


if __name__ == '__main__':
    sys.exit(main())
