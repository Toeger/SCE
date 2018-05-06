virtualenv venv -p $1 > setuplog && . venv/bin/activate && $1 -m pip install --upgrade pip >> setuplog && $1 -m pip install grpcio >> setuplog && $1 rpc_call.py
