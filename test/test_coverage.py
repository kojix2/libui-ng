import gzip
import json
from pathlib import Path
import tempfile
import unittest

from coverage import summarize


def function(name, hit):
    return {'name': name, 'start_line': 7, 'start_column': 1,
            'end_line': 7, 'end_column': 1, 'execution_count': hit}


def line(name, counts):
    return {'line_number': 7, 'function_name': name, 'count': max(counts),
            'branches': [{'count': count, 'fallthrough': index % 2 == 0,
                          'throw': False} for index, count in enumerate(counts)]}


class CoverageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.repo = self.root / 'repo'

    def report(self, name, functions, lines, source='common/macro.c'):
        record = {'file': str(self.repo / source), 'functions': functions, 'lines': lines}
        data = {'current_working_directory': str(self.repo), 'files': [record]}
        with gzip.open(self.root / (name + '.gcov.json.gz'), 'wt') as stream:
            json.dump(data, stream)

    def test_duplicate_copies_keep_functions_generated_on_one_line(self):
        self.report('native', [function('first', 1), function('second', 0)],
                    [line('first', [1, 0]), line('second', [0, 0, 0, 0])])
        self.report('included', [function('first', 1), function('second', 1)],
                    [line('first', [0, 1]), line('second', [1, 0, 1, 0])])
        for index, source in enumerate(('test/case.c', 'windows/inactive.c', 'generated/version.c')):
            self.report('excluded' + str(index), [function('irrelevant', 1)],
                        [line('irrelevant', [1, 1])], source)
        result = summarize(self.root, self.repo, 'unix')
        self.assertEqual(result['source_records'], 2)
        self.assertEqual(result['unique_sources'], 1)
        self.assertEqual(result['summary']['combined'], {
            'lines': {'hit': 1, 'total': 1},
            'functions': {'hit': 2, 'total': 2},
            'branches': {'hit': 4, 'total': 6},
        })

    def test_unexecuted_functions_and_branches_remain_in_totals(self):
        self.report('unexecuted', [function('missing', 0)], [line('missing', [0, 0])])
        result = summarize(self.root, self.repo, 'unix')['summary']['combined']
        self.assertEqual(result, {
            'lines': {'hit': 0, 'total': 1},
            'functions': {'hit': 0, 'total': 1},
            'branches': {'hit': 0, 'total': 2},
        })

    def test_incompatible_branch_lists_are_rejected(self):
        self.report('native', [function('same', 1)], [line('same', [1, 0])])
        self.report('variant', [function('same', 1)], [line('same', [1, 0, 1, 0])])
        with self.assertRaisesRegex(ValueError, 'incompatible branch lists'):
            summarize(self.root, self.repo, 'unix')


if __name__ == '__main__':
    unittest.main()
