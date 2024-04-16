# Copyright (C) 2013-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from __future__ import absolute_import

import perftest.testresult as testresult
import perftest.reporter as reporter
from perftest.measure import Measure
from perftest.measure import MeasurementPerfCounter
from perftest.measure import MeasurementProcessTime
from perftest.measure import MeasurementWallTime
from perftest.measure import MeasurementVmSize


class TestCase(object):
    """Base class of all performance testing cases.

    Each sub-class should override methods execute_test, in which
    several GDB operations are performed and measured by attribute
    measure.  Sub-class can also override method warm_up optionally
    if the test case needs warm up.
    """

    def __init__(self, name, measure):
        """Constructor of TestCase.

        Construct an instance of TestCase with a name and a measure
        which is to measure the test by several different measurements.
        """

        self.name = name
        self.measure = measure

    def execute_test(self):
        """Abstract method to do the actual tests."""
        raise NotImplementedError("Abstract Method.")

    def warm_up(self):
        """Do some operations to warm up the environment."""
        pass

    def run(self, warm_up=True, append=True):
        """Run this test case.

        It is a template method to invoke method warm_up,
        execute_test, and finally report the measured results.
        If parameter warm_up is True, run method warm_up.  If parameter
        append is True, the test result will be appended instead of
        overwritten.
        """
        if warm_up:
            self.warm_up()

        self.execute_test()
        self.measure.report(reporter.TextReporter(append), self.name)


class TestCaseWithBasicMeasurements(TestCase):
    """Test case measuring CPU time, wall time and memory usage."""

    def __init__(self, name):
        result_factory = testresult.SingleStatisticResultFactory()
        measurements = [
            MeasurementPerfCounter(result_factory.create_result()),
            MeasurementProcessTime(result_factory.create_result()),
            MeasurementWallTime(result_factory.create_result()),
            MeasurementVmSize(result_factory.create_result()),
        ]
        super(TestCaseWithBasicMeasurements, self).__init__(name, Measure(measurements))
