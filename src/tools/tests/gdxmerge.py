import unittest
import os
import subprocess
import gams.transfer as gt
from examples.small_example import create_small_example
from examples.full_example import create_full_example


def run_gdxmerge(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [os.path.join('..', '..', '..', 'build', 'gdxmerge'), *command],
        capture_output=True,
        text=True
    )


class TestGdxMerge(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        create_small_example(os.path.join('.', 'examples', 'small_example.gdx'))
        create_full_example(os.path.join('.', 'examples', 'full_example.gdx'))

    @classmethod
    def tearDownClass(cls):
        os.remove(os.path.join('.', 'examples', 'small_example.gdx'))
        os.remove(os.path.join('.', 'examples', 'full_example.gdx'))
        os.remove(os.path.join('.', 'examples', 'merged.gdx'))

    def test_small_and_full_example(self):
        output = run_gdxmerge([
            os.path.join('.', 'examples', 'small_example.gdx'),
            os.path.join('.', 'examples', 'full_example.gdx'),
            f'output={os.path.join('.', 'examples', 'merged.gdx')}'
        ])
        self.assertEqual(output.returncode, 0)
        with open(os.path.join('.', 'output', 'gdxmerge', 'small_and_full_example.txt'), 'r') as file:
            first = output.stdout.split('\n')[3:]
            second = file.read().split('\n')[3:]
            self.assertEqual(first, second)
        self.assertEqual(output.stderr, '')

    def test_small_and_full_example_gdx_file(self):
        container = gt.Container(load_from=os.path.join('.', 'examples', 'merged.gdx'))
        self.assertIn('i', container)
        self.assertIn('j', container)
        self.assertIn('d', container)
        self.assertIn('a', container)
        self.assertIn('b', container)
        self.assertIn('f', container)
        self.assertIn('c', container)
        self.assertIn('x', container)
        self.assertIn('z', container)
        self.assertIn('cost', container)
        self.assertIn('supply', container)
        self.assertIn('demand', container)
        self.assertIn('Merged_set_1', container)
        self.assertEqual(len(container), 13)

        symbol: gt.Parameter = container['i']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['small_example', 'seattle', ''],
            ['small_example', 'san-diego', ''],
            ['full_example', 'seattle', ''],
            ['full_example', 'san-diego', '']
        ]
        self.assertEqual(first, second)

        symbol = container['j']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['small_example', 'new-york', ''],
            ['small_example', 'chicago', ''],
            ['small_example', 'topeka', ''],
            ['full_example', 'new-york', ''],
            ['full_example', 'chicago', ''],
            ['full_example', 'topeka', '']
        ]
        self.assertEqual(first, second)

        symbol = container['d']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['small_example', 'seattle', 'new-york', 2.5],
            ['small_example', 'seattle', 'chicago', 1.7],
            ['small_example', 'seattle', 'topeka', 1.8],
            ['small_example', 'san-diego', 'new-york', 2.5],
            ['small_example', 'san-diego', 'chicago', 1.8],
            ['small_example', 'san-diego', 'topeka', 1.4],
            ['full_example', 'seattle', 'new-york', 2.5],
            ['full_example', 'seattle', 'chicago', 1.7],
            ['full_example', 'seattle', 'topeka', 1.8],
            ['full_example', 'san-diego', 'new-york', 2.5],
            ['full_example', 'san-diego', 'chicago', 1.8],
            ['full_example', 'san-diego', 'topeka', 1.4]
        ]
        self.assertEqual(first, second)

        symbol = container['a']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['full_example', 'seattle', 350.0],
            ['full_example', 'san-diego', 600.0]
        ]
        self.assertEqual(first, second)

        symbol = container['b']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['full_example', 'new-york', 325.0],
            ['full_example', 'chicago', 300.0],
            ['full_example', 'topeka', 275.0]
        ]
        self.assertEqual(first, second)

        symbol = container['f']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['full_example', 90.0]
        ]
        self.assertEqual(first, second)

        symbol = container['c']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['full_example', 'seattle', 'new-york', 0.225],
            ['full_example', 'seattle', 'chicago', 0.153],
            ['full_example', 'seattle', 'topeka', 0.162],
            ['full_example', 'san-diego', 'new-york', 0.225],
            ['full_example', 'san-diego', 'chicago', 0.162],
            ['full_example', 'san-diego', 'topeka', 0.12599999999999997]
        ]
        self.assertEqual(first, second)

        symbol = container['x']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['full_example', 'seattle', 'new-york', 50.0, 0.0, 0.0, float('inf'), 1.0],
            ['full_example', 'seattle', 'chicago', 300.0, 0.0, 0.0, float('inf'), 1.0],
            ['full_example', 'seattle', 'topeka', 0.0, 0.036, 0.0, float('inf'), 1.0],
            ['full_example', 'san-diego', 'new-york', 275.0, 0.0, 0.0, float('inf'), 1.0],
            ['full_example', 'san-diego', 'chicago', 0.0, 0.009, 0.0, float('inf'), 1.0],
            ['full_example', 'san-diego', 'topeka', 275.0, 0.0, 0.0, float('inf'), 1.0]
        ]
        self.assertEqual(first, second)

        symbol = container['z']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['full_example', 153.675, 0.0, float('-inf'), float('inf'), 1.0]
        ]
        self.assertEqual(first, second)

        symbol = container['cost']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['full_example', 0.0, 1.0, 0.0, 0.0, 1.0]
        ]
        self.assertEqual(first, second)

        symbol = container['supply']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['full_example', 'seattle', 350.0, 0.0, float('-inf'), 350.0, 1.0],
            ['full_example', 'san-diego', 550.0, 0.0, float('-inf'), 600.0, 1.0]
        ]
        self.assertEqual(first, second)

        symbol = container['demand']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['full_example', 'new-york', 325.0, 0.225, 325.0, float('inf'), 1.0],
            ['full_example', 'chicago', 300.0, 0.153, 300.0, float('inf'), 1.0],
            ['full_example', 'topeka', 275.0, 0.126, 275.0, float('inf'), 1.0]
        ]
        self.assertEqual(first, second)

        symbol = container['Merged_set_1']  # type: ignore
        first = symbol.records.values.tolist()
        second = [
            ['small_example', r'\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}  \.[/\\]examples[/\\]small_example.gdx'],
            ['full_example', r'\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}  \.[/\\]examples[/\\]full_example.gdx']
        ]
        self.assertEqual(len(first), 2)
        for item in first:
            self.assertEqual(len(item), 2)
        self.assertEqual(first[0][0], second[0][0])
        self.assertRegex(first[0][1], second[0][1])
        self.assertEqual(first[1][0], second[1][0])
        self.assertRegex(first[1][1], second[1][1])
