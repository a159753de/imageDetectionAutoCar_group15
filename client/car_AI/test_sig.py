from hubconf import custom
import inspect

print(inspect.getsource(custom))
print("Signature:", inspect.signature(custom))
