import aiohttp

_session: aiohttp.ClientSession | None = None

async def get_session() -> aiohttp.ClientSession:
    global _session
    if _session is None:
        _session = aiohttp.ClientSession()
    return _session

async def close_session():
    global _session
    if _session is not None:
        await _session.close()
        _session = None
