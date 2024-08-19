import unittest
import sys
import gams


class TestGdxMerge(unittest.TestCase):

    def test_gams_module(self):
        self.assertTrue('gams' in sys.modules)

    def test_gams_api(self):
        self.assertRegex(
            f'API OK -- Version {gams.__version__}',
            r'API OK -- Version [0-9]+\.[0-9]+\.[0-9]+'
        )
