#!/usr/bin/env python3
"""
YAML-based code generator for PfoMonData and PfoMonDataCollection.
Reads PfoMonData.yaml and generates the corresponding C++ headers.

Usage:
    python3 generate_pfo_monitoring.py <yaml_file> <output_dir>
"""

import os
import sys
import yaml

TYPE_MAP = {
    'float': 'float',
    'int32_t': 'int32_t',
    'uint32_t': 'uint32_t',
    'int64_t': 'int64_t',
    'uint64_t': 'uint64_t',
    'double': 'double',
    'bool': 'bool',
    'string': 'std::string',
}


def generate_data_header(config):
    ns = config['namespace']
    members = config['members']

    members_lines = []
    getters = []
    setters = []
    private_members = []

    # Group members by type for compact declaration in the private section
    float_vars = []
    int32_vars = []
    uint32_vars = []

    for m in members:
        name = m['name']
        m_type = m['type']
        camel = name[0].upper() + name[1:]

        # Getter
        getters.append(f'    {m_type} get{camel}() const {{ return m_{name}; }}')

        # Setter
        setters.append(f'    void set{camel}({m_type} v) {{ m_{name} = v; }}')

        # Private member
        if m_type == 'float':
            float_vars.append(f'm_{name}')
        elif m_type == 'int32_t':
            int32_vars.append(f'm_{name}')
        elif m_type == 'uint32_t':
            uint32_vars.append(f'm_{name}')
        else:
            private_members.append(f'    {m_type} m_{name}{{0}};')

    # Build compact private declarations
    if float_vars:
        private_members.insert(0, f'    float {"{0}, ".join(float_vars)}{{0}};')
    if int32_vars:
        private_members.insert(0, f'    int32_t {"{0}, ".join(int32_vars)}{{0}};')
    if uint32_vars:
        private_members.insert(0, f'    uint32_t {"{0}, ".join(uint32_vars)}{{0}};')

    lines = []
    lines.append(f'#ifndef GAUDIPFOMONITORING_PFOMONDATA_H')
    lines.append(f'#define GAUDIPFOMONITORING_PFOMONDATA_H')
    lines.append('')
    lines.append('#include <cstdint>')
    lines.append('')
    lines.append(f'namespace {ns} {{')
    lines.append('')
    lines.append('class PfoMonData {')
    lines.append('public:')
    lines.append('    PfoMonData() = default;')
    lines.append('')
    lines.append('    // Getters')
    for g in getters:
        lines.append(g)
    lines.append('')
    lines.append('    // Setters')
    for s in setters:
        lines.append(s)
    lines.append('')
    lines.append('private:')
    for p in private_members:
        lines.append(p)
    lines.append('};')
    lines.append('')
    lines.append(f'}} // namespace {ns}')
    lines.append('')
    lines.append('#endif')
    lines.append('')
    return '\n'.join(lines)


def generate_collection_header(config):
    ns = config['namespace']
    clid = config['collection_id']

    lines = []
    lines.append(f'#ifndef GAUDIPFOMONITORING_PFOMONDATACOLLECTION_H')
    lines.append(f'#define GAUDIPFOMONITORING_PFOMONDATACOLLECTION_H')
    lines.append('')
    lines.append('#include "GaudiKernel/DataObject.h"')
    lines.append('#include "PfoMonData.h"')
    lines.append('#include <vector>')
    lines.append('')
    lines.append(f'namespace {ns} {{')
    lines.append('')
    lines.append(f'// Unique ID for the Gaudi Event Store')
    lines.append(f'static const CLID CLID_PfoMonDataCollection = {clid};')
    lines.append('')
    lines.append('class PfoMonDataCollection : public DataObject, public std::vector<PfoMonData> {')
    lines.append('public:')
    lines.append('    PfoMonDataCollection() = default;')
    lines.append('    virtual ~PfoMonDataCollection() = default;')
    lines.append('')
    lines.append('    // Mimic PODIO\'s create method')
    lines.append('    PfoMonData& create() {')
    lines.append('        this->emplace_back();')
    lines.append('        return this->back();')
    lines.append('    }')
    lines.append('')
    lines.append('    // Gaudi DataObject requirements')
    lines.append('    static const CLID& classID() {{ return CLID_PfoMonDataCollection; }}')
    lines.append('    virtual const CLID& clID() const {{ return classID(); }}')
    lines.append('};')
    lines.append('')
    lines.append(f'}} // namespace {ns}')
    lines.append('')
    lines.append('#endif')
    lines.append('')
    return '\n'.join(lines)


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <yaml_file> <output_dir>", file=sys.stderr)
        sys.exit(1)

    yaml_file = sys.argv[1]
    output_dir = sys.argv[2]

    with open(yaml_file, 'r') as f:
        config = yaml.safe_load(f)

    os.makedirs(output_dir, exist_ok=True)

    # Generate PfoMonData.h
    data_header = generate_data_header(config)
    data_path = os.path.join(output_dir, 'PfoMonData.h')
    with open(data_path, 'w') as f:
        f.write(data_header)
    print(f"Generated: {data_path}")

    # Generate PfoMonDataCollection.h
    collection_header = generate_collection_header(config)
    collection_path = os.path.join(output_dir, 'PfoMonDataCollection.h')
    with open(collection_path, 'w') as f:
        f.write(collection_header)
    print(f"Generated: {collection_path}")


if __name__ == '__main__':
    main()