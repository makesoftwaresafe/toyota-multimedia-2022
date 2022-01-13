#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (c) 2009 Per Øyvind Karlsen <peroyvind@mandriva.org>
#
import unittest
from test.test_support import rmtree
import rpm


class Test_evrCompare(unittest.TestCase):


    def test_e(self):
        self.assertEqual(rpm.evrCompare("1", "2"), -1)
        self.assertEqual(rpm.evrCompare("3", "2"), 1)
        self.assertEqual(rpm.evrCompare("4", "4"), 0)
        self.assertEqual(rpm.evrCompare("5", "20"), -1)

    def test_ev(self):
        self.assertEqual(rpm.evrCompare("1:32", "2:231"), -1)
        self.assertEqual(rpm.evrCompare("3:1.1", "2:5"), 1)
        self.assertEqual(rpm.evrCompare("2:1.1", "2:0.1"), 1)
        self.assertEqual(rpm.evrCompare("4:123", "4:123"), 0)
        self.assertEqual(rpm.evrCompare("5:1.3", "20:9.3"), -1)

    def test_evr(self):
        self.assertEqual(rpm.evrCompare("1:3.2-6", "2:9.4-99"), -1)
        self.assertEqual(rpm.evrCompare("3:3-1", "2:9.3"), 1)
        self.assertEqual(rpm.evrCompare("4:429-999999", "4:0.1-2"), 1)
        self.assertEqual(rpm.evrCompare("5:23-83:23", "20:0.0.1-0.1"), -1)

    def test_evrd(self):
        self.assertEqual(rpm.evrCompare("10:321.32a-p21:999", "2:99"), 1)
        self.assertEqual(rpm.evrCompare("3", "2:531-9:1"), -1)
        self.assertEqual(rpm.evrCompare("4:3-2:1", "4:3-2"), 1)
        self.assertEqual(rpm.evrCompare("20:9-3:2011.0", "20:9-3:2011.0"), 0)

def test_main():
    from test import test_support
    test_support.run_unittest(
	    Test_evrCompare)
    test_support.reap_children()


if __name__ == "__main__":
    test_main()
