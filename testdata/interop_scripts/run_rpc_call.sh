virtualenv venv -p $1 > setuplog 2>setuperr && . venv/bin/activate && $1 -m pip install --upgrade pip >> setuplog 2>>setuperr && $1 -m pip install grpcio >> setuplog 2>>setuperr && $1 rpc_call.py
