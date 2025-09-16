# MQTT Info
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
          "avg_power": {
            "factor": 0,
            "active": 0,
            "reactive": 0,
            "apparent": 0
          },
          "avg_energy": {
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
          },
          "avg_power": {
            "factor": 0,
            "active": 0,
            "reactive": 0,
            "apparent": 0
          },
          "avg_energy": {
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
          },
          "avg_power": {
            "factor": 0,
            "active": 0,
            "reactive": 0,
            "apparent": 0
          },
          "avg_energy": {
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
            "command": true
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
            "device_pass": "abc123",
            "device_imei": "865788062388016",
            "device_ip": "10.99.72.185",
            "mqtt_status": true,
            "sftp_status": false
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
## MQTT Info (Existing but needs to be changed as per MQTTs)
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
## SFTP Info (Existing but needs to be changed)
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
