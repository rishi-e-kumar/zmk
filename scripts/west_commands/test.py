# Copyright (c) 2020 The ZMK Contributors
#
# SPDX-License-Identifier: MIT
'''Test runner for ZMK.'''

import os
import subprocess
from textwrap import dedent            # just for nicer code indentation

from west.commands import WestCommand
from west import log                   # use this for user output


class Test(WestCommand):
    def __init__(self):
        super().__init__(
            'test',  # gets stored as self.name
            'run ZMK testsuite',  # self.help
            # self.description:
            dedent('''Run the ZMK testsuite.'''))

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(self.name,
                                         help=self.help,
                                         description=self.description)

        parser.add_argument('test_path', default="all",
                            help='The path to the test. Defaults to "all".', nargs="?")
        return parser           # gets stored as self.parser

    def do_run(self, args, unknown_args):
        # the run-test script assumes the zmk directory is the current dir.
        os.chdir(f'{self.topdir}/zmk')
        completed_process = subprocess.run(
            [f'{self.topdir}/zmk/run-test.sh', args.test_path])
        exit(completed_process.returncode)