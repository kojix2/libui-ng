#!/usr/bin/env python3
"""Summarize GCC gcov JSON for common/ and one backend, merging source copies."""

import argparse
import gzip
import json
from pathlib import Path


def summarize(report_dir, source_root, backend):
    sources = {}
    reports = sorted(report_dir.glob('*.gcov.json.gz'))
    if not reports:
        raise ValueError('no *.gcov.json.gz reports found')
    for report in reports:
        with gzip.open(report, 'rt', encoding='utf-8') as stream:
            data = json.load(stream)
        for record in data['files']:
            path = Path(record['file'])
            if not path.is_absolute():
                path = Path(data['current_working_directory']) / path
            try:
                relative = path.resolve().relative_to(source_root.resolve())
            except ValueError:
                continue
            if relative.parts[0] not in ('common', backend):
                continue
            source = sources.setdefault(str(relative), {
                'copies': 0, 'lines': {}, 'functions': {}, 'branches': {},
                'branch_shapes': {},
            })
            source['copies'] += 1
            identities = {}
            for function in record['functions']:
                # Macro-generated functions can share ALL source coordinates.
                # Keep their names; current fault tests rename callees only.
                identity = tuple(function[key] for key in (
                    'name', 'start_line', 'start_column', 'end_line', 'end_column'))
                identities[function['name']] = identity
                hit = function['execution_count'] > 0
                source['functions'][identity] = source['functions'].get(identity, False) or hit
            for line in record['lines']:
                number = line['line_number']
                source['lines'][number] = source['lines'].get(number, False) or line['count'] > 0
                if not line['branches']:
                    continue
                identity = identities[line['function_name']]
                key = (identity, number)
                shape = tuple((edge['fallthrough'], edge['throw']) for edge in line['branches'])
                previous = source['branch_shapes'].setdefault(key, shape)
                if previous != shape:
                    raise ValueError(f'incompatible branch lists: {relative}:{number} '
                                     f'in {line["function_name"]}')
                # GCC block IDs shift between native and fault-test copies.
                # Within one function/line, matching edge lists retain order.
                for index, edge in enumerate(line['branches']):
                    branch = (identity, number, index)
                    source['branches'][branch] = source['branches'].get(branch, False) or edge['count'] > 0
    if not sources:
        raise ValueError('no production records under the selected source root/backend')

    def counts(records):
        return {metric: {
            'hit': sum(sum(source[metric].values()) for source in records),
            'total': sum(len(source[metric]) for source in records),
        } for metric in ('lines', 'functions', 'branches')}

    return {
        'backend': backend,
        'input_reports': len(reports),
        'source_records': sum(source['copies'] for source in sources.values()),
        'unique_sources': len(sources),
        'summary': {
            scope: counts([source for path, source in sources.items()
                           if scope == 'combined' or Path(path).parts[0] == scope])
            for scope in ('common', backend, 'combined')
        },
        'sources': {path: {'copies': source['copies'], **counts([source])}
                    for path, source in sorted(sources.items())},
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report_dir', type=Path)
    parser.add_argument('--source-root', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--backend', choices=('unix', 'windows', 'darwin'), required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = summarize(args.report_dir, args.source_root, args.backend)
    except (ValueError, KeyError) as error:
        parser.error(str(error))
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result['summary'], indent=2))


if __name__ == '__main__':
    main()
