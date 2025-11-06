# MQTTs Info
## Telemetry Data 
### Method
- Publish
### Structure
```
{
  "timestamp": "2025-07-24T00:33:00+05:00",
  "device_uuid": "ESP90000005",
  "channels": [
    {
      "ID": 1,
      "status": <boolean>,
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
          }
        }
      }
    }
  ]
}
```
## Reboot
### Method
- Publish
- Subscribe
### Structure
```
{
    "device_uuid": "ESP90000005",
    "data":[
        {
            "command": <boolean>
        }
    ]
}
```
## About
### Method
- Publish
### Structure
```
{
    "device_uuid":  <string>,
    "data":[
        {
            "Manufacturer": "EIN Technology Labs SDN. BHD.",
            "Hardware Version":  <string>,
            "Firmware Version":  <string>,
            "QT License Version": <string>
        }
    ]
}
```
## Device Info
### Method
- Publish
### Structure
```
{
    "device_uuid": "ESP90000005",
    "data":[
        {
            "device_imei": "865788062388016",
            "device_ip": "10.99.72.185",
            "mqtt_status": <boolean>,
            "sftp_status": <boolean>,
            "ntp_status": <string>,
            "gsm_status": <string>,
        }
    ]
}
```
## NTP Info
### Method
- Publish
- Subscribe
### Structure
```
{
    "device_uuid": "ESP90000005",
    "data": [
        {
            "server_1": "my.pool.ntp.org",
            "server_2": "0.asia.pool.ntp.org",
            "server_3": "1.asia.pool.ntp.org",
        }
    ]
}
```
## MQTT Info
### Method
- Publish
- Subscribe
### Structure
```
{
  "device_uuid": "ESP90000005",
  "data":[
    {
        "broker_ip": "thingsboard.cloud",
        "broker_port": 1883
        "broker_user": "OFP-INDKOM",
        "broker_pass": "IND10",
        "data_interval" : 900,
        "mqtt_topic": "v1/devices/me/telemetry"
    }
  ]
}
```
## SFTP Info
### Method
- Publish
- Subscribe
### Structure
```
{
    "device_uuid": "ESP90000005",
    "data": [
      {
          "server_ip": "ap-southeast-1.sftpcloud.io",
          "server_port": 22,
          "username": "ofp-msm",
          "password": "root123456",
          "data_interval": 900
      }
    ]
}
```
## MQTTs Info
### Method
- Publish
- Subscribe
### Structure
```
{
    "device_uuid": "ESP90000005",
    "data": [
      {
        "broker_ip": "a1dxcxg260l7m-ats.iot.eu-north-1.amazonaws.com",
        "broker_port": 8883,
        "data_interval": 10,
        "credentials": {
            "root_ca": <boolean>,
            "claim_cert": <boolean>,
            "claim_key": <boolean>
        }
      }
    ]
}
```
## OTA Info
### Method
- Publish
### Structure
```
{
    "device_uuid": "ESP90000005",
    "data": [  
      {
        "current_version": "2.0.7",
        "last_update": {
            "date": "2025-10-28",
            "time": "16:02:37"
        }
      }
    ]
}
```
## OTA Trigger
### Structure
```
{
    "device_uuid": "ESP90000005",
    "data": [  
      {
        "ota_version": "2.0.8",
      }
    ]
}
```
## Measurements
### CT-Ratio
#### Method
- Publish
- Subscribe
#### Structure
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
    "channel-7": 100,
    "channel-8": 200,
    "channel-9": 500,
    "channel-10": 200,
    "channel-11": 200,
    "channel-12": 300
  }
}
```
### V-Offset
#### Method
- Publish
- Subscribe
#### Structure
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
### I-Offset
#### Method
- Publish
- Subscribe
#### Structure
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
