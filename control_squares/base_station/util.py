
HOST='localhost'
PORT=8080

def make_absolute_url( relative_url: str  ):
    return f'http://{HOST}:{PORT}{relative_url}'