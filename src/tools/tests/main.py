import sys
import argparse
import unittest
from gdxdump import TestGdxDump
from gdxdiff import TestGdxDiff
from gdxmerge import TestGdxMerge


def main() -> int:
    parser = argparse.ArgumentParser(description='gdxtools tests')
    parser.add_argument('-t', '--tool', help='specify the tool you want to test')
    args = parser.parse_args()

    def suite():
        suite = unittest.TestSuite()

        match args.tool:
            case 'gdxdump' | 'dump':
                suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestGdxDump))

            case 'gdxdiff' | 'diff':
                suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestGdxDiff))

            case 'gdxmerge' | 'merge':
                suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestGdxMerge))

            case _:
                suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestGdxDump))
                suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestGdxDiff))
                suite.addTests(unittest.TestLoader().loadTestsFromTestCase(TestGdxMerge))

        return suite

    runner = unittest.TextTestRunner()
    result = runner.run(suite())

    if result.wasSuccessful():
        return 0
    else:
        return 1


if __name__ == '__main__':
    sys.exit(main())
