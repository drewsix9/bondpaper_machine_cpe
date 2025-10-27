import time

def millis():
  """
  Returns the current time in milliseconds since the epoch (January 1, 1970).

  This function provides a Python equivalent of the Arduino `millis()` function.
  """
  return round(time.time() * 1000) 