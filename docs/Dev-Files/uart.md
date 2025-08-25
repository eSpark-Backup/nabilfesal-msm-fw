# UART Info
## Channel Data 
### Transmitting
```
channel-N\n
```
**Note: N is set of natural numbers till 12.**
### Receiving
```
{
  "ID": 1,
  "status": true,
  "temperature": 27.5,
  "data": {
    "phase_a": {
      "general": {	
        "line_voltage": 0,
        "rms_voltage": 0,
        "frequency": 0,
        "current": 0
      },
      "power": {
        "factor": 0,
        "active": 0,
        "reactive": 0,
        "apparent": 0
      },
      "energy": {
        "active": {
          "positive": 0,
          "negative": 0
        },
        "reactive": {
          "positive": 0,
          "negative": 0
        }
      },
      "threshold": {
        "current": 0,
        "voltage": 0,
        "angle": 0
      }
    },
    "phase_b": {
      "general": {
        "line_voltage": 0,
        "rms_voltage": 0,
        "frequency": 0,
        "current": 0
      },
      "power": {
        "factor": 0,
        "active": 0,
        "reactive": 0,
        "apparent": 0
      },
      "energy": {
        "active": {
          "positive": 0,
          "negative": 0
        },
        "reactive": {
          "positive": 0,
          "negative": 0
        }
      },
      "threshold": {
        "current": 0,
        "voltage": 0,
        "angle": 0
      }
    },
    "phase_c": {
      "general": {
        "line_voltage": 0,
        "rms_voltage": 0,
        "frequency": 0,
        "current": 0
      },
      "power": {
        "factor": 0,
        "active": 0,
        "reactive": 0,
        "apparent": 0
      },
      "energy": {
        "active": {
          "positive": 0,
          "negative": 0
        },
        "reactive": {
          "positive": 0,
          "negative": 0
        }
      },
      "threshold": {
        "current": 0,
        "voltage": 0,
        "angle": 0
      }
    }
  }
}
```
## CT Ratio
### Transmitting
```
{
  "command": "set-ct-ratio",
  "ct_ratio": {
    "channel-1": 200,
    "channel-2": 200,
    "channel-3": 200,
    "channel-4": 70,
    "channel-5": 200,
    "channel-6": 80,
    "channel-7": 200,
    "channel-8": 200,
    "channel-9": 500,
    "channel-10": 200,
    "channel-11": 200,
    "channel-12": 300
  }
}
```
### Receiving
```
"CT Ratio updated successfully!"
```
## Current Offset
### Transmitting
```
{
  "command": "set-i-offset",
  "i_offset": {
    "channel-1": 200,
    "channel-2": 200,
    "channel-3": 200,
    "channel-4": 70,
    "channel-5": 200,
    "channel-6": 80,
    "channel-7": 200,
    "channel-8": 200,
    "channel-9": 500,
    "channel-10": 200,
    "channel-11": 200,
    "channel-12": 300
  }
}
```
### Receiving
```
"Current Offset updated successfully!"
```
## Voltage Offset
### Transmitting
```
{
  "command": "set-v-offset",
  "v_offset": {
    "channel-1": 200,
    "channel-2": 200,
    "channel-3": 200,
    "channel-4": 70,
    "channel-5": 200,
    "channel-6": 80,
    "channel-7": 200,
    "channel-8": 200,
    "channel-9": 500,
    "channel-10": 200,
    "channel-11": 200,
    "channel-12": 300
  }
}
```
### Receiving
```
"Voltage Offset updated successfully!"
```