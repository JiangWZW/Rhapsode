"""Unit test for llm._retry_complete: retries on exception AND on empty return,
while preserving the contracts (persistent exception raises; persistent empty
returns ""). No API key or network -- drives the wrapper with fake callables.

Run:  python server/test_llm_retry.py
"""

import logging
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

from rhapsode import llm

# Make retries instant.
llm.time.sleep = lambda *_a, **_k: None

failures = []


def check(cond, msg):
    print(f"  [{'ok  ' if cond else 'FAIL'}] {msg}")
    if not cond:
        failures.append(msg)


class Counter:
    """Callable that records how many times it was invoked."""
    def __init__(self, behavior):
        self.behavior = behavior  # fn(attempt_index) -> str | raises
        self.calls = 0

    def __call__(self):
        i = self.calls
        self.calls += 1
        return self.behavior(i)


# (a) non-empty immediately -> returned, called once
c = Counter(lambda i: "hello")
out = llm._retry_complete(c)
check(out == "hello" and c.calls == 1, "non-empty returns immediately, no retry")

# (b) always empty -> called _MAX_RETRIES times, returns "" (no raise)
c = Counter(lambda i: "")
out = llm._retry_complete(c)
check(out == "" and c.calls == llm._MAX_RETRIES,
      f"persistent empty retried {llm._MAX_RETRIES}x and returned ''")

# (c) always raises -> raises on final attempt, called _MAX_RETRIES times
def _boom(i):
    raise RuntimeError("api down")
c = Counter(_boom)
raised = False
try:
    llm._retry_complete(c)
except RuntimeError:
    raised = True
check(raised and c.calls == llm._MAX_RETRIES,
      f"persistent exception retried {llm._MAX_RETRIES}x then raised")

# (d) empty then non-empty -> returns the non-empty value
c = Counter(lambda i: "" if i == 0 else "recovered")
out = llm._retry_complete(c)
check(out == "recovered" and c.calls == 2, "empty-then-nonempty recovers on retry")

# (e) the empty path logs a warning
records = []
handler = logging.Handler()
handler.emit = lambda r: records.append(r)
llm.log.addHandler(handler)
llm.log.setLevel(logging.WARNING)
llm._retry_complete(Counter(lambda i: ""))
llm.log.removeHandler(handler)
check(any("empty" in r.getMessage().lower() for r in records),
      "empty retries emit a warning log")

print()
if failures:
    print(f"FAILED: {len(failures)} assertion(s)")
    sys.exit(1)
print("All _retry_complete assertions passed.")
