"""Composition root: builds the FastAPI app, warms models on startup, mounts the
read-only graph/inspection endpoints, and exposes the /ws session loop.

Turn sequencing lives in the C++ engine (advance_player + complete_turn); Python is a thin
transport + LLM-adapter layer, split across the modules this file wires together:
  - config       : paths + logging
  - llm_tools    : narrator tool schemas + the LLM tool-use adapter
  - scheduler    : scheduler tool schemas + the LLM tool-use adapter
  - graph_views  : HTTP inspection endpoints (router)
  - session      : engine construction, output streaming, and the /ws loop
"""

from contextlib import asynccontextmanager

from fastapi import FastAPI, WebSocket

from rhapsode.config import configure_logging
from rhapsode.memory import warmup_model
from rhapsode.fable import warmup_fable
from rhapsode.graph_views import router as graph_router
from rhapsode.session import run_session

configure_logging()


@asynccontextmanager
async def lifespan(application: FastAPI):
    warmup_model()
    warmup_fable()
    yield


app = FastAPI(title="Rhapsode", lifespan=lifespan)
app.include_router(graph_router)


@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    await run_session(ws)
