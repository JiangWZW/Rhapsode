from concurrent.futures import ThreadPoolExecutor

from rhapsode.llm_tools import PromptJobs


class _Job:
    def __init__(self, handle, prompt, staging_buf_id, generation=0):
        self.handle = handle
        self.prompt = prompt
        self.staging_buf_id = staging_buf_id
        self.generation = generation


def test_ready_is_none_until_done_then_pops():
    def call(prompt):
        return prompt + "!"

    with ThreadPoolExecutor(max_workers=1) as pool:
        jobs = PromptJobs(call, pool)
        jobs.submit([_Job(1, "hi", 0)])
        raw = jobs.ready(1, 0)
        while raw is None:
            raw = jobs.ready(1, 0)
        assert raw == ("hi!", False)
        assert jobs.ready(1, 0) is None


def test_ready_marks_exception_failed():
    def call(_prompt):
        raise RuntimeError("boom")

    with ThreadPoolExecutor(max_workers=1) as pool:
        jobs = PromptJobs(call, pool)
        jobs.submit([_Job(2, "x", 1)])
        raw = jobs.ready(2, 1)
        while raw is None:
            raw = jobs.ready(2, 1)
        assert raw == ("", True)


def test_wait_blocks_until_done():
    def call(prompt):
        return prompt

    with ThreadPoolExecutor(max_workers=1) as pool:
        jobs = PromptJobs(call, pool)
        jobs.submit([_Job(3, "ok", 0)])
        jobs.wait()
        assert jobs.ready(3, 0) == ("ok", False)


def test_ready_rejects_mismatched_generation():
    def call(prompt):
        return prompt

    with ThreadPoolExecutor(max_workers=1) as pool:
        jobs = PromptJobs(call, pool)
        jobs.submit([_Job(4, "x", 0, generation=2)])
        jobs.wait()
        assert jobs.ready(4, 0, 1) is None
        assert jobs.ready(4, 0, 2) == ("x", False)
