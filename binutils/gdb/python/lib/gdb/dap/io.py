# Copyright 2022-2024 Free Software Foundation, Inc.

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

import json

from .startup import start_thread, send_gdb, log


def read_json(stream):
    """Read a JSON-RPC message from STREAM.
    The decoded object is returned."""
    # First read and parse the header.
    content_length = None
    while True:
        line = stream.readline()
        line = line.strip()
        if line == b"":
            break
        if line.startswith(b"Content-Length:"):
            line = line[15:].strip()
            content_length = int(line)
            continue
        log("IGNORED: <<<%s>>>" % line)
    data = bytes()
    while len(data) < content_length:
        new_data = stream.read(content_length - len(data))
        data += new_data
    result = json.loads(data)
    return result


def start_json_writer(stream, queue):
    """Start the JSON writer thread.
    It will read objects from QUEUE and write them to STREAM,
    following the JSON-RPC protocol."""

    def _json_writer():
        seq = 1
        while True:
            obj = queue.get()
            if obj is None:
                # This is an exit request.  The stream is already
                # flushed, so all that's left to do is request an
                # exit.
                send_gdb("quit")
                break
            obj["seq"] = seq
            seq = seq + 1
            encoded = json.dumps(obj)
            body_bytes = encoded.encode("utf-8")
            header = "Content-Length: " + str(len(body_bytes)) + "\r\n\r\n"
            header_bytes = header.encode("ASCII")
            stream.write(header_bytes)
            stream.write(body_bytes)
            stream.flush()

    start_thread("JSON writer", _json_writer)
