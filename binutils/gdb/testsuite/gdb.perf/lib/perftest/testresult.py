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


class TestResult(object):
    """Base class to record and report test results.

    Method record is to record the results of test case, and report
    method is to report the recorded results by a given reporter.
    """

    def record(self, parameter, result):
        raise NotImplementedError("Abstract Method:record.")

    def report(self, reporter, name):
        """Report the test results by reporter."""
        raise NotImplementedError("Abstract Method:report.")


class SingleStatisticTestResult(TestResult):
    """Test results for the test case with a single statistic."""

    def __init__(self):
        super(SingleStatisticTestResult, self).__init__()
        self.results = dict()

    def record(self, parameter, result):
        if parameter in self.results:
            self.results[parameter].append(result)
        else:
            self.results[parameter] = [result]

    def report(self, reporter, name):
        reporter.start()
        for key in sorted(self.results.keys()):
            reporter.report(name, key, self.results[key])
        reporter.end()


class ResultFactory(object):
    """A factory to create an instance of TestResult."""

    def create_result(self):
        """Create an instance of TestResult."""
        raise NotImplementedError("Abstract Method:create_result.")


class SingleStatisticResultFactory(ResultFactory):
    """A factory to create an instance of SingleStatisticTestResult."""

    def create_result(self):
        return SingleStatisticTestResult()
