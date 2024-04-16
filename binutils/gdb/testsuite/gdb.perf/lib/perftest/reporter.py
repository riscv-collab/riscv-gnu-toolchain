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

# Text reports are written here.
# This is the perftest counterpart to gdb.sum.
SUM_FILE_NAME = "perftest.sum"

# Raw data that went into the report is written here.
# This is the perftest counterpart to gdb.log.
LOG_FILE_NAME = "perftest.log"


class Reporter(object):
    """Base class of reporter to report test results in a certain format.

    Subclass, which is specific to a report format, should overwrite
    methods report, start and end.
    """

    def __init__(self, append):
        """Constructor of Reporter.

        attribute append is used to determine whether to append or
        overwrite log file.
        """
        self.append = append

    def report(self, *args):
        raise NotImplementedError("Abstract Method:report.")

    def start(self):
        """Invoked when reporting is started."""
        raise NotImplementedError("Abstract Method:start.")

    def end(self):
        """Invoked when reporting is done.

        It must be overridden to do some cleanups, such as closing file
        descriptors.
        """
        raise NotImplementedError("Abstract Method:end.")


class TextReporter(Reporter):
    """Report results in a plain text file 'perftest.log'."""

    def __init__(self, append):
        super(TextReporter, self).__init__(Reporter(append))
        self.txt_sum = None
        self.txt_log = None

    def report(self, test_name, measurement_name, data_points):
        if len(data_points) == 0:
            self.txt_sum.write(
                "%s %s *no data recorded*\n" % (test_name, measurement_name)
            )
            return
        average = sum(data_points) / len(data_points)
        data_min = min(data_points)
        data_max = max(data_points)
        self.txt_sum.write("%s %s %s\n" % (test_name, measurement_name, average))
        self.txt_log.write(
            "%s %s %s, min %s, max %s, data %s\n"
            % (test_name, measurement_name, average, data_min, data_max, data_points)
        )

    def start(self):
        mode = "a+" if self.append else "w"
        self.txt_sum = open(SUM_FILE_NAME, mode)
        self.txt_log = open(LOG_FILE_NAME, mode)

    def end(self):
        self.txt_sum.close()
        self.txt_log.close()
