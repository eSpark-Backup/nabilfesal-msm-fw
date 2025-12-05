/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* POSIX includes. */
#include <unistd.h>
#include <errno.h>

// #define DOWNLOADED_CERT_WRITE_PATH "/home/esparkdev/Downloads/device-crts/downloadcert.pem"
// #define DOWNLOADED_PRIVATE_KEY_WRITE_PATH "/home/esparkdev/Downloads/device-crts/downloadpkey.pem"
#include <fcntl.h>

// #if defined( DOWNLOADED_CERT_WRITE_PATH ) || defined( DOWNLOADED_PRIVATE_KEY_WRITE_PATH )
//     #include <fcntl.h>
// #endif

/* Demo config. */
#include "demo_config.h"
#include "app_config.h"

/* corePKCS11 includes. */
#include "core_pkcs11.h"
#include "core_pkcs11_config.h"

/* AWS IoT Fleet Provisioning Library. */
#include "shadow.h"
#include "fleet_provisioning.h"

/* Demo includes. */
#include "mqtt_operations.h"
#include "pkcs11_operations.h"
#include "fleet_provisioning_serializer.h"

#include "core_json.h"

/**
 * These configurations are required. Throw compilation error if it is not
 * defined.
 */
#ifndef PROVISIONING_TEMPLATE_NAME
    #error "Please define PROVISIONING_TEMPLATE_NAME to the template name registered with AWS IoT Core in demo_config.h."
#endif
#ifndef CLAIM_CERT_PATH
    #error "Please define path to claim certificate (CLAIM_CERT_PATH) in demo_config.h."
#endif
#ifndef CLAIM_PRIVATE_KEY_PATH
    #error "Please define path to claim private key (CLAIM_PRIVATE_KEY_PATH) in demo_config.h."
#endif
#ifndef DEVICE_SERIAL_NUMBER
    #error "Please define a serial number (DEVICE_SERIAL_NUMBER) in demo_config.h."
#endif

app_config_t gConfig;

/**
 * @brief The length of #PROVISIONING_TEMPLATE_NAME.
 */
#define PROVISIONING_TEMPLATE_NAME_LENGTH    ( ( uint16_t ) ( sizeof( PROVISIONING_TEMPLATE_NAME ) - 1 ) )

/**
 * @brief The length of #DEVICE_SERIAL_NUMBER.
 */
#define DEVICE_SERIAL_NUMBER_LENGTH          ( ( uint16_t ) ( sizeof( DEVICE_SERIAL_NUMBER ) - 1 ) )

/**
 * @brief Size of AWS IoT Thing name buffer.
 *
 * See https://docs.aws.amazon.com/iot/latest/apireference/API_CreateThing.html#iot-CreateThing-request-thingName
 */
#define MAX_THING_NAME_LENGTH                128

/**
 * @brief The maximum number of times to run the loop in this demo.
 *
 * @note The demo loop is attempted to re-run only if it fails in an iteration.
 * Once the demo loop succeeds in an iteration, the demo exits successfully.
 */
#ifndef FLEET_PROV_MAX_DEMO_LOOP_COUNT
    #define FLEET_PROV_MAX_DEMO_LOOP_COUNT    ( 3 )
#endif

/**
 * @brief Time in seconds to wait between retries of the demo loop if
 * demo loop fails.
 */
#define DELAY_BETWEEN_DEMO_RETRY_ITERATIONS_SECONDS    ( 5 )

/**
 * @brief Size of buffer in which to hold the private key.
 */
#define PRIV_KEY_BUFFER_LENGTH                         2048

/**
 * @brief Size of buffer in which to hold the certificate.
 */
#define CERT_BUFFER_LENGTH                             2048

/**
 * @brief Size of buffer in which to hold the certificate id.
 *
 * See https://docs.aws.amazon.com/iot/latest/apireference/API_Certificate.html#iot-Type-Certificate-certificateId
 */
#define CERT_ID_BUFFER_LENGTH                          64

/**
 * @brief Size of buffer in which to hold the certificate ownership token.
 */
#define OWNERSHIP_TOKEN_BUFFER_LENGTH                  512

/**
 * @brief Status values of the Fleet Provisioning response.
 */
typedef enum
{
    ResponseNotReceived,
    ResponseAccepted,
    ResponseRejected
} ResponseStatus_t;

/*-----------------------------------------------------------*/

/**
 * @brief Status reported from the MQTT publish callback.
 */
static ResponseStatus_t responseStatus;

/**
 * @brief Buffer to hold the provisioned AWS IoT Thing name.
 */
static char thingName[ MAX_THING_NAME_LENGTH ];

/**
 * @brief Length of the AWS IoT Thing name.
 */
static size_t thingNameLength;

/**
 * @brief Buffer to hold responses received from the AWS IoT Fleet Provisioning
 * APIs. When the MQTT publish callback receives an expected Fleet Provisioning
 * accepted payload, it copies it into this buffer.
 */
static uint8_t payloadBuffer[ NETWORK_BUFFER_SIZE ];

/**
 * @brief Length of the payload stored in #payloadBuffer. This is set by the
 * MQTT publish callback when it copies a received payload into #payloadBuffer.
 */
static size_t payloadLength;

/*-----------------------------------------------------------*/

/**
 * @brief Callback to receive the incoming publish messages from the MQTT
 * broker. Sets responseStatus if an expected CreateKeysAndCertificate or
 * RegisterThing response is received, and copies the response into
 * responseBuffer if the response is an accepted one.
 *
 * @param[in] pPublishInfo Pointer to publish info of the incoming publish.
 * @param[in] packetIdentifier Packet identifier of the incoming publish.
 */
static void provisioningPublishCallback( MQTTPublishInfo_t * pPublishInfo,
                                         uint16_t packetIdentifier );

/**
 * @brief Run the MQTT process loop to get a response.
 */
static bool waitForResponse( void );

/**
 * @brief Subscribe to the CreateKeysAndCertificate accepted and rejected topics.
 */
static bool subscribeToKeysCertResponseTopics( void );

/**
 * @brief Unsubscribe from the CreateKeysAndCertificate accepted and rejected topics.
 */
static bool unsubscribeFromKeysCertResponseTopics( void );

/**
 * @brief Subscribe to the RegisterThing accepted and rejected topics.
 */
static bool subscribeToRegisterThingResponseTopics( void );

/**
 * @brief Unsubscribe from the RegisterThing accepted and rejected topics.
 */
static bool unsubscribeFromRegisterThingResponseTopics( void );

/*-----------------------------------------------------------*/

static void provisioningPublishCallback( MQTTPublishInfo_t * pPublishInfo,
                                         uint16_t packetIdentifier )
{
    FleetProvisioningStatus_t status;
    FleetProvisioningTopic_t api;
    const char * cborDump;

    /* Silence compiler warnings about unused variables. */
    ( void ) packetIdentifier;

    status = FleetProvisioning_MatchTopic( pPublishInfo->pTopicName,
                                           pPublishInfo->topicNameLength, &api );

    if( status != FleetProvisioningSuccess )
    {
        LogWarn( ( "Unexpected publish message received. Topic: %.*s.",
                   ( int ) pPublishInfo->topicNameLength,
                   ( const char * ) pPublishInfo->pTopicName ) );
    }
    else
    {
        if( api == FleetProvCborCreateKeysAndCertAccepted )
        {
            LogInfo( ( "Received accepted response from Fleet Provisioning CreateKeysAndCertificate API." ) );

            cborDump = getStringFromCbor( ( const uint8_t * ) pPublishInfo->pPayload, pPublishInfo->payloadLength );
            LogDebug( ( "Payload: %s", cborDump ) );
            free( ( void * ) cborDump );

            responseStatus = ResponseAccepted;

            /* Copy the payload from the MQTT library's buffer to #payloadBuffer. */
            ( void ) memcpy( ( void * ) payloadBuffer,
                             ( const void * ) pPublishInfo->pPayload,
                             ( size_t ) pPublishInfo->payloadLength );

            payloadLength = pPublishInfo->payloadLength;
        }
        else if( api == FleetProvCborCreateKeysAndCertRejected )
        {
            LogError( ( "Received rejected response from Fleet Provisioning CreateKeysAndCertificate API." ) );

            cborDump = getStringFromCbor( ( const uint8_t * ) pPublishInfo->pPayload, pPublishInfo->payloadLength );
            LogError( ( "Payload: %s", cborDump ) );
            free( ( void * ) cborDump );

            responseStatus = ResponseRejected;
        }
        else if( api == FleetProvCborRegisterThingAccepted )
        {
            LogInfo( ( "Received accepted response from Fleet Provisioning RegisterThing API." ) );

            cborDump = getStringFromCbor( ( const uint8_t * ) pPublishInfo->pPayload, pPublishInfo->payloadLength );
            LogDebug( ( "Payload: %s", cborDump ) );
            free( ( void * ) cborDump );

            responseStatus = ResponseAccepted;

            /* Copy the payload from the MQTT library's buffer to #payloadBuffer. */
            ( void ) memcpy( ( void * ) payloadBuffer,
                             ( const void * ) pPublishInfo->pPayload,
                             ( size_t ) pPublishInfo->payloadLength );

            payloadLength = pPublishInfo->payloadLength;
        }
        else if( api == FleetProvCborRegisterThingRejected )
        {
            LogError( ( "Received rejected response from Fleet Provisioning RegisterThing API." ) );

            cborDump = getStringFromCbor( ( const uint8_t * ) pPublishInfo->pPayload, pPublishInfo->payloadLength );
            LogError( ( "Payload: %s", cborDump ) );
            free( ( void * ) cborDump );

            responseStatus = ResponseRejected;
        }
        else
        {
            LogError( ( "Received message on unexpected Fleet Provisioning topic. Topic: %.*s.",
                        ( int ) pPublishInfo->topicNameLength,
                        ( const char * ) pPublishInfo->pTopicName ) );
        }
    }
}
/*-----------------------------------------------------------*/

static bool waitForResponse( void )
{
    bool status = false;

    responseStatus = ResponseNotReceived;

    /* responseStatus is updated from the MQTT publish callback. */
    ( void ) ProcessLoopWithTimeout();

    if( responseStatus == ResponseNotReceived )
    {
        LogError( ( "Timed out waiting for response." ) );
    }

    if( responseStatus == ResponseAccepted )
    {
        status = true;
    }

    return status;
}
/*-----------------------------------------------------------*/

static bool subscribeToKeysCertResponseTopics( void )
{
    bool status;

    status = SubscribeToTopic( FP_CBOR_CREATE_KEYS_ACCEPTED_TOPIC,
                               FP_CBOR_CREATE_KEYS_ACCEPTED_LENGTH );

    if( status == false )
    {
        LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
                    FP_CBOR_CREATE_KEYS_ACCEPTED_LENGTH,
                    FP_CBOR_CREATE_KEYS_ACCEPTED_TOPIC ) );
    }

    if( status == true )
    {
        status = SubscribeToTopic( FP_CBOR_CREATE_KEYS_REJECTED_TOPIC,
                                   FP_CBOR_CREATE_KEYS_REJECTED_LENGTH );

        if( status == false )
        {
            LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
                        FP_CBOR_CREATE_KEYS_REJECTED_LENGTH,
                        FP_CBOR_CREATE_KEYS_REJECTED_TOPIC ) );
        }
    }

    return status;
}
/*-----------------------------------------------------------*/

static bool unsubscribeFromKeysCertResponseTopics( void )
{
    bool status;

    status = UnsubscribeFromTopic( FP_CBOR_CREATE_KEYS_ACCEPTED_TOPIC,
                                   FP_CBOR_CREATE_KEYS_ACCEPTED_LENGTH );

    if( status == false )
    {
        LogError( ( "Failed to unsubscribe from fleet provisioning topic: %.*s.",
                    FP_CBOR_CREATE_KEYS_ACCEPTED_LENGTH,
                    FP_CBOR_CREATE_KEYS_ACCEPTED_TOPIC ) );
    }

    if( status == true )
    {
        status = UnsubscribeFromTopic( FP_CBOR_CREATE_KEYS_REJECTED_TOPIC,
                                       FP_CBOR_CREATE_KEYS_REJECTED_LENGTH );

        if( status == false )
        {
            LogError( ( "Failed to unsubscribe from fleet provisioning topic: %.*s.",
                        FP_CBOR_CREATE_KEYS_REJECTED_LENGTH,
                        FP_CBOR_CREATE_KEYS_REJECTED_TOPIC ) );
        }
    }

    return status;
}
/*-----------------------------------------------------------*/
// #ifdef PROVISIONING_TEMPLATE_NAME
//     #undef PROVISIONING_TEMPLATE_NAME
// #endif

// #define PROVISIONING_TEMPLATE_NAME (gConfig.template_name)

char topicBuffer[256];
#define AWS_PROVISIONING_ACCEPTED_TOPIC(buffer, size, template_name) \
    snprintf((buffer), (size), \
             "$aws/provisioning-templates/%s/provision/json/accepted", \
             (template_name))

#define AWS_PROVISIONING_REJECTED_TOPIC(buffer, size, template_name) \
    snprintf((buffer), (size), \
             "$aws/provisioning-templates/%s/provision/json/rejected", \
             (template_name))

#define AWS_PROVISIONING_PUBLISH_TOPIC(buffer, size, template_name) \
    snprintf((buffer), (size), \
             "$aws/provisioning-templates/%s/provision/cbor", \
             (template_name))

static bool subscribeToRegisterThingResponseTopics( void )
{
    bool status;
    AWS_PROVISIONING_ACCEPTED_TOPIC(topicBuffer, sizeof(topicBuffer), gConfig.template_name);
    // status = SubscribeToTopic( FP_CBOR_REGISTER_ACCEPTED_TOPIC( PROVISIONING_TEMPLATE_NAME ),
    //                            FP_CBOR_REGISTER_ACCEPTED_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ) );
    status = SubscribeToTopic( ( topicBuffer ),
                               ( strlen(topicBuffer) ) );


    if( status == false )
    {
        // LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
        //             FP_CBOR_REGISTER_ACCEPTED_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ),
        //             FP_CBOR_REGISTER_ACCEPTED_TOPIC( PROVISIONING_TEMPLATE_NAME ) ) );
        LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
                    ( (int)strlen(topicBuffer) ),
                    ( topicBuffer) ) );        
    }

    if( status == true )
    {
        AWS_PROVISIONING_REJECTED_TOPIC(topicBuffer, sizeof(topicBuffer), gConfig.template_name);
        // status = SubscribeToTopic( FP_CBOR_REGISTER_REJECTED_TOPIC( PROVISIONING_TEMPLATE_NAME ),
        //                            FP_CBOR_REGISTER_REJECTED_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ) );
        status = SubscribeToTopic( ( topicBuffer ),
                            ( strlen(topicBuffer) ));

        if( status == false )
        {
            // LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
            //             FP_CBOR_REGISTER_REJECTED_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ),
            //             FP_CBOR_REGISTER_REJECTED_TOPIC( PROVISIONING_TEMPLATE_NAME ) ) );
            LogError( ( "Failed to subscribe to fleet provisioning topic: %.*s.",
                    ( (int)strlen(topicBuffer) ),
                    ( topicBuffer) ) ); 
        }
    }

    return status;
}
/*-----------------------------------------------------------*/

static bool unsubscribeFromRegisterThingResponseTopics( void )
{
    bool status;

    // status = UnsubscribeFromTopic( FP_CBOR_REGISTER_ACCEPTED_TOPIC( PROVISIONING_TEMPLATE_NAME ),
    //                                FP_CBOR_REGISTER_ACCEPTED_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ) );

    AWS_PROVISIONING_ACCEPTED_TOPIC(topicBuffer, sizeof(topicBuffer), gConfig.template_name);
    status = UnsubscribeFromTopic( topicBuffer,
                                   ( strlen(topicBuffer) ));

    if( status == false )
    {
        // LogError( ( "Failed to unsubscribe from fleet provisioning topic: %.*s.",
        //             FP_CBOR_REGISTER_ACCEPTED_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ),
        //             FP_CBOR_REGISTER_ACCEPTED_TOPIC( PROVISIONING_TEMPLATE_NAME ) ) );
        LogError( ( "Failed to unsubscribe to fleet provisioning topic: %.*s.",
                ( (int)strlen(topicBuffer) ),
                ( topicBuffer) ) );   
    }

    if( status == true )
    {
        // status = UnsubscribeFromTopic( FP_CBOR_REGISTER_REJECTED_TOPIC( PROVISIONING_TEMPLATE_NAME ),
        //                                FP_CBOR_REGISTER_REJECTED_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ) );
        AWS_PROVISIONING_REJECTED_TOPIC(topicBuffer, sizeof(topicBuffer), gConfig.template_name);
        status = UnsubscribeFromTopic( topicBuffer,
                                   ( strlen(topicBuffer) ));
        if( status == false )
        {
            // LogError( ( "Failed to unsubscribe from fleet provisioning topic: %.*s.",
            //             FP_CBOR_REGISTER_REJECTED_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ),
            //             FP_CBOR_REGISTER_REJECTED_TOPIC( PROVISIONING_TEMPLATE_NAME ) ) );
            LogError( ( "Failed to unsubscribe to fleet provisioning topic: %.*s.",
                ( (int)strlen(topicBuffer) ),
                ( topicBuffer) ) ); 
        }
    }

    return status;
}
/*-----------------------------------------------------------*/

bool loadConfig(const char *path);

void initializeDeviceShadow( void );

/* Define your Thing Name */
#define THING_NAME       gConfig.serial
#define THING_NAME_LENGTH gConfig.serial_len

/* Shadow topics buffer size */
#define SHADOW_TOPIC_BUFFER_SIZE 128
#define SHADOW_BUFFER_SIZE       512

size_t outLength;

/* Buffers */
static char shadowUpdateTopic[ SHADOW_TOPIC_BUFFER_SIZE ];
static char shadowUpdateAcceptedTopic[ SHADOW_TOPIC_BUFFER_SIZE ];
static char shadowUpdateRejectedTopic[ SHADOW_TOPIC_BUFFER_SIZE ];
static char shadowPayload[ SHADOW_BUFFER_SIZE ];

/* This example uses a single application task, which shows that how to use
 * the Fleet Provisioning library to generate and validate AWS IoT Fleet
 * Provisioning MQTT topics, and use the coreMQTT library to communicate with
 * the AWS IoT Fleet Provisioning APIs. */
int main( int argc,
          char ** argv )
{
    bool status = false;
    /* Buffer for holding received certificate until it is saved. */
    char certificate[ CERT_BUFFER_LENGTH ];
    size_t certificateLength;
    /* Buffer for holding received private key until it is saved. */
    char privatekey[ PRIV_KEY_BUFFER_LENGTH ];
    size_t privatekeyLength;
    /* Buffer for holding the certificate ID. */
    char certificateId[ CERT_ID_BUFFER_LENGTH ];
    size_t certificateIdLength;
    /* Buffer for holding the certificate ownership token. */
    char ownershipToken[ OWNERSHIP_TOKEN_BUFFER_LENGTH ];
    size_t ownershipTokenLength;
    bool connectionEstablished = false;
    CK_SESSION_HANDLE p11Session;
    int demoRunCount = 0;
    CK_RV pkcs11ret = CKR_OK;

    /* Silence compiler warnings about unused variables. */
    ( void ) argc;
    ( void ) argv;

    if (argc < 2) { printf("Usage: %s <config.json>\n", argv[0]); return -1; } 
    const char *configPath = argv[1]; printf("Loading config file: %s\n", configPath); 
    loadConfig(configPath);

    // loadConfig("config.json");

    do
    {
        /* Initialize the buffer lengths to their max lengths. */
        certificateLength = CERT_BUFFER_LENGTH;
        privatekeyLength = PRIV_KEY_BUFFER_LENGTH;
        certificateIdLength = CERT_ID_BUFFER_LENGTH;
        ownershipTokenLength = OWNERSHIP_TOKEN_BUFFER_LENGTH;

        /* Initialize the PKCS #11 module */
        pkcs11ret = xInitializePkcs11Session( &p11Session );

        if( pkcs11ret != CKR_OK )
        {
            LogError( ( "Failed to initialize PKCS #11." ) );
            status = false;
        }
        else
        {
            /* Insert the claim credentials into the PKCS #11 module */
            // status = loadClaimCredentials( p11Session,
            //                                CLAIM_CERT_PATH,
            //                                pkcs11configLABEL_CLAIM_CERTIFICATE,
            //                                CLAIM_PRIVATE_KEY_PATH,
            //                                pkcs11configLABEL_CLAIM_PRIVATE_KEY );
            status = loadClaimCredentials( p11Session,
                                            gConfig.claim_cert,
                                            pkcs11configLABEL_CLAIM_CERTIFICATE,
                                            gConfig.claim_key,
                                            pkcs11configLABEL_CLAIM_PRIVATE_KEY );

            if( status == false )
            {
                LogError( ( "Failed to provision PKCS #11 with claim credentials." ) );
            }
        }

        /**** Connect to AWS IoT Core with provisioning claim credentials *****/

        /* We first use the claim credentials to connect to the broker. These
         * credentials should allow use of the RegisterThing API and one of the
         * CreateCertificatefromCsr or CreateKeysAndCertificate.
         * In this demo we use CreateKeysAndCertificate. */

        if( status == true )
        {
            /* Attempts to connect to the AWS IoT MQTT broker. If the
             * connection fails, retries after a timeout. Timeout value will
             * exponentially increase until maximum attempts are reached. */
            LogInfo( ( "Establishing MQTT session with claim certificate..." ) );
            status = EstablishMqttSession( provisioningPublishCallback,
                                           p11Session,
                                           pkcs11configLABEL_CLAIM_CERTIFICATE,
                                           pkcs11configLABEL_CLAIM_PRIVATE_KEY );

            if( status == false )
            {
                LogError( ( "Failed to establish MQTT session." ) );
            }
            else
            {
                LogInfo( ( "Established connection with claim credentials." ) );
                connectionEstablished = true;
            }
        }

        /**** Call the CreateKeysAndCertificate API ***************************/

        /* We use the CreateKeysAndCertificate API to obtain a client certificate
         * and private key. */
        if( status == true )
        {
            /* Subscribe to the CreateKeysAndCertificate accepted and rejected
             * topics. In this demo we use CBOR encoding for the payloads,
             * so we use the CBOR variants of the topics. */
            status = subscribeToKeysCertResponseTopics();
        }

        if( status == true )
        {
            /* Publish an empty payload to the CreateKeysAndCertificate API. */
            status = PublishToTopic( FP_CBOR_CREATE_KEYS_PUBLISH_TOPIC,
                                     FP_CBOR_CREATE_KEYS_PUBLISH_LENGTH,
                                     "",
                                     0 );

            if( status == false )
            {
                LogError( ( "Failed to publish to fleet provisioning topic: %.*s.",
                            FP_CBOR_CREATE_KEYS_PUBLISH_LENGTH,
                            FP_CBOR_CREATE_KEYS_PUBLISH_TOPIC ) );
            }
        }

        if( status == true )
        {
            /* Get the response to the CreateKeysAndCertificate request. */
            status = waitForResponse();
        }

        if( status == true )
        {
            /* From the response, extract the certificate, certificate ID, and
             * certificate ownership token. */
            status = parseKeyCertResponse( payloadBuffer,
                                           payloadLength,
                                           certificate,
                                           &certificateLength,
                                           privatekey,
                                           &privatekeyLength,
                                           certificateId,
                                           &certificateIdLength,
                                           ownershipToken,
                                           &ownershipTokenLength );

            if( status == true )
            {
                LogInfo( ( "Received privatekey and certificate with Id: %.*s", ( int ) certificateIdLength, certificateId ) );
            }
        }

        if( status == true )
        {
            /* Save the private key into PKCS #11. */
            status = loadPrivateKey( p11Session,
                                     privatekey,
                                     pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS,
                                     privatekeyLength );
        }

        if( status == true )
        {
            /* Save the certificate into PKCS #11. */
            status = loadCertificate( p11Session,
                                      certificate,
                                      pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS,
                                      certificateLength );
        }

        if( status == true )
        {
            /* Unsubscribe from the CreateKeysAndCertificate topics. */
            status = unsubscribeFromKeysCertResponseTopics();
        }

        /**** Call the RegisterThing API **************************************/

        /* We then use the RegisterThing API to activate the received certificate,
         * provision AWS IoT resources according to the provisioning template, and
         * receive device configuration. */
        if( status == true )
        {
            /* Create the request payload to publish to the RegisterThing API. */
            // status = generateRegisterThingRequest( payloadBuffer,
            //                                        NETWORK_BUFFER_SIZE,
            //                                        ownershipToken,
            //                                        ownershipTokenLength,
            //                                        DEVICE_SERIAL_NUMBER,
            //                                        DEVICE_SERIAL_NUMBER_LENGTH,
            //                                        &payloadLength );
            status = generateRegisterThingRequest( payloadBuffer,
                                                   NETWORK_BUFFER_SIZE,
                                                   ownershipToken,
                                                   ownershipTokenLength,
                                                   gConfig.serial,
                                                   gConfig.serial_len,
                                                   &payloadLength );
        }

        if( status == true )
        {
            /* Subscribe to the RegisterThing response topics. */
            status = subscribeToRegisterThingResponseTopics();
        }

        if( status == true )
        {
            /* Publish the RegisterThing request. */
            // status = PublishToTopic( FP_CBOR_REGISTER_PUBLISH_TOPIC( PROVISIONING_TEMPLATE_NAME ),
            //                          FP_CBOR_REGISTER_PUBLISH_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ),
            //                          ( char * ) payloadBuffer,
            //                          payloadLength );
            AWS_PROVISIONING_PUBLISH_TOPIC(topicBuffer, sizeof(topicBuffer), gConfig.template_name);
            status = PublishToTopic( ( topicBuffer ),
                                     ( strlen(topicBuffer) ),
                                     ( char * ) payloadBuffer,
                                     payloadLength );

            if( status == false )
            {
                // LogError( ( "Failed to publish to fleet provisioning topic: %.*s.",
                //             FP_CBOR_REGISTER_PUBLISH_LENGTH( PROVISIONING_TEMPLATE_NAME_LENGTH ),
                //             FP_CBOR_REGISTER_PUBLISH_TOPIC( PROVISIONING_TEMPLATE_NAME ) ) );
                LogError( ( "Failed to publish to fleet provisioning topic: %.*s.",
                        ( (int)strlen(topicBuffer) ),
                        ( topicBuffer) ) );                
            }
        }

        if( status == true )
        {
            /* Get the response to the RegisterThing request. */
            status = waitForResponse();
        }

        if( status == true )
        {
            /* Extract the Thing name from the response. */
            thingNameLength = MAX_THING_NAME_LENGTH;
            status = parseRegisterThingResponse( payloadBuffer,
                                                 payloadLength,
                                                 thingName,
                                                 &thingNameLength );

            if( status == true )
            {
                LogInfo( ( "Received AWS IoT Thing name: %.*s", ( int ) thingNameLength, thingName ) );
            }
        }

        if( status == true )
        {
            /* Unsubscribe from the RegisterThing topics. */
            unsubscribeFromRegisterThingResponseTopics();
        }

        initializeDeviceShadow();

        /**** Disconnect from AWS IoT Core ************************************/

        /* As we have completed the provisioning workflow, we disconnect from
         * the connection using the provisioning claim credentials. We will
         * establish a new MQTT connection with the newly provisioned
         * credentials. */
        if( connectionEstablished == true )
        {
            DisconnectMqttSession();
            connectionEstablished = false;
        }

        /**** Connect to AWS IoT Core with provisioned certificate ************/

        if( status == true )
        {
            LogInfo( ( "Establishing MQTT session with provisioned certificate..." ) );
            status = EstablishMqttSession( provisioningPublishCallback,
                                           p11Session,
                                           pkcs11configLABEL_DEVICE_CERTIFICATE_FOR_TLS,
                                           pkcs11configLABEL_DEVICE_PRIVATE_KEY_FOR_TLS );

            if( status != true )
            {
                LogError( ( "Failed to establish MQTT session with provisioned "
                            "credentials. Verify on your AWS account that the "
                            "new certificate is active and has an attached IoT "
                            "Policy that allows the \"iot:Connect\" action." ) );
            }
            else
            {
                LogInfo( ( "Sucessfully established connection with provisioned credentials." ) );
                connectionEstablished = true;
                // sleep(60);
            }
        }

        /**** Finish **********************************************************/

        if( connectionEstablished == true )
        {
            /* Close the connection. */
            DisconnectMqttSession();
            connectionEstablished = false;
        }

        pkcs11CloseSession( p11Session );

        /**** Retry in case of failure ****************************************/

        /* Increment the demo run count. */
        demoRunCount++;

        if( status == true )
        {
            LogInfo( ( "Demo iteration %d is successful.", demoRunCount ) );
        }
        /* Attempt to retry a failed iteration of demo for up to #FLEET_PROV_MAX_DEMO_LOOP_COUNT times. */
        else if( demoRunCount < FLEET_PROV_MAX_DEMO_LOOP_COUNT )
        {
            LogWarn( ( "Demo iteration %d failed. Retrying...", demoRunCount ) );
            sleep( DELAY_BETWEEN_DEMO_RETRY_ITERATIONS_SECONDS );
        }
        /* Failed all #FLEET_PROV_MAX_DEMO_LOOP_COUNT demo iterations. */
        else
        {
            LogError( ( "All %d demo iterations failed.", FLEET_PROV_MAX_DEMO_LOOP_COUNT ) );
            break;
        }
    } while( status != true );

    /* Log demo success. */
    if( status == true )
    {
        LogInfo( ( "Demo completed successfully." ) );

        // #if defined( DOWNLOADED_CERT_WRITE_PATH )
        if((strlen(gConfig.prov_cert_path) != 0))
            {
                int fd = open( gConfig.prov_cert_path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR );

                if( -1 != fd )
                {
                    const ssize_t writtenBytes = write( fd, certificate, certificateLength );

                    if( writtenBytes == certificateLength )
                    {
                        LogInfo( ( "Written %s successfully.", gConfig.prov_cert_path ) );
                    }
                    else
                    {
                        LogError( ( "Could not write to %s. Error: %s.", gConfig.prov_cert_path, strerror( errno ) ) );
                    }

                    close( fd );
                }
                else
                {
                    LogError( ( "Could not open %s. Error: %s.", gConfig.prov_cert_path, strerror( errno ) ) );
                }
            }
        else 
            {
                LogError( ( "File Path for prov_cert_path not provided - %s", gConfig.prov_cert_path ) );
            }
        // #else /* if defined( DOWNLOADED_CERT_WRITE_PATH ) */
        //     LogInfo( ( "NOTE: define DOWNLOADED_CERT_WRITE_PATH in order to have the certificate written to disk." ) );
        // #endif // DOWNLOADED_CERT_WRITE_PATH

        // #if defined( DOWNLOADED_PRIVATE_KEY_WRITE_PATH )
        if((strlen(gConfig.prov_key_path) != 0))
            {
                int fd = open( gConfig.prov_key_path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR );

                if( -1 != fd )
                {
                    const ssize_t writtenBytes = write( fd, privatekey, privatekeyLength );

                    if( writtenBytes == privatekeyLength )
                    {
                        LogInfo( ( "Written %s successfully.", gConfig.prov_key_path ) );
                    }
                    else
                    {
                        LogError( ( "Could not write to %s. Error: %s.", gConfig.prov_key_path, strerror( errno ) ) );
                    }

                    close( fd );
                }
                else
                {
                    LogError( ( "Could not open %s. Error: %s.", gConfig.prov_key_path, strerror( errno ) ) );
                }
            }
        else
            {
                LogError( ( "File Path for prov_key_path not provided - %s", gConfig.prov_key_path ) );
            }
        // #else /* if defined( DOWNLOADED_PRIVATE_KEY_WRITE_PATH ) */
        //     LogInfo( ( "NOTE: define DOWNLOADED_PRIVATE_KEY_WRITE_PATH in order to have the private key written to disk." ) );
        // #endif // DOWNLOADED_PRIVATE_KEY_WRITE_PATH
    }

    return ( status == true ) ? EXIT_SUCCESS : EXIT_FAILURE;
}
/*-----------------------------------------------------------*/

MQTTContext_t mqttContext;
/* Callback when messages arrive */
static void eventCallback( MQTTContext_t * pMqttContext,
                           MQTTPublishInfo_t * pPublishInfo )
{
    ( void ) pMqttContext;

    if( pPublishInfo->topicNameLength > 0 )
    {
        if( strncmp( pPublishInfo->pTopicName, shadowUpdateAcceptedTopic,
                     pPublishInfo->topicNameLength ) == 0 )
        {
            LogInfo( ( "✅ Shadow update accepted. Payload: %.*s",
                       pPublishInfo->payloadLength,
                       ( const char * ) pPublishInfo->pPayload ) );
        }
        else if( strncmp( pPublishInfo->pTopicName, shadowUpdateRejectedTopic,
                          pPublishInfo->topicNameLength ) == 0 )
        {
            LogError( ( "❌ Shadow update rejected. Payload: %.*s",
                        pPublishInfo->payloadLength,
                        ( const char * ) pPublishInfo->pPayload ) );
        }
    }
}

void initializeDeviceShadow( void )
{
    ShadowStatus_t shadowStatus;
    int written;
    size_t outLength = 0;
    bool result;

    /* Build update topic */
    shadowStatus = Shadow_AssembleTopicString(
        ShadowTopicStringTypeUpdate,
        gConfig.serial,
        gConfig.serial_len,
        NULL, /* classic (unnamed) shadow */
        0,
        shadowUpdateTopic,
        SHADOW_TOPIC_BUFFER_SIZE,
        &outLength
    );

    if( shadowStatus != SHADOW_SUCCESS )
    {
        LogError( ( "Failed to assemble Update topic string. Error=%d", shadowStatus ) );
        return;
    }

    /* Build update/accepted topic */
    shadowStatus = Shadow_AssembleTopicString(
        ShadowTopicStringTypeUpdateAccepted,
        gConfig.serial,
        gConfig.serial_len,
        NULL,
        0,
        shadowUpdateAcceptedTopic,
        SHADOW_TOPIC_BUFFER_SIZE,
        &outLength
    );

    if( shadowStatus != SHADOW_SUCCESS )
    {
        LogError( ( "Failed to assemble UpdateAccepted topic string. Error=%d", shadowStatus ) );
        return;
    }

    /* Build update/rejected topic */
    shadowStatus = Shadow_AssembleTopicString(
        ShadowTopicStringTypeUpdateRejected,
        gConfig.serial,
        gConfig.serial_len,
        NULL,
        0,
        shadowUpdateRejectedTopic,
        SHADOW_TOPIC_BUFFER_SIZE,
        &outLength
    );

    if( shadowStatus != SHADOW_SUCCESS )
    {
        LogError( ( "Failed to assemble UpdateRejected topic string. Error=%d", shadowStatus ) );
        return;
    }

    /* Subscribe to accepted topic */
    result = SubscribeToTopic( shadowUpdateAcceptedTopic,
                               ( uint16_t ) strlen( shadowUpdateAcceptedTopic ) );
    if( !result )
    {
        LogError( ( "Failed to subscribe to update/accepted topic." ) );
        return;
    }

    /* Subscribe to rejected topic */
    result = SubscribeToTopic( shadowUpdateRejectedTopic,
                               ( uint16_t ) strlen( shadowUpdateRejectedTopic ) );
    if( !result )
    {
        LogError( ( "Failed to subscribe to update/rejected topic." ) );
        return;
    }

    LogInfo( ( "Subscribed to shadow update accepted/rejected topics." ) );

    /* Prepare initial shadow document using gConfig.serial */
    written = snprintf( shadowPayload, SHADOW_BUFFER_SIZE,
                        "{ \"state\": {"
                        "   \"desired\": {"
                        "       \"deviceType\": \"IoTDevice\","
                        "       \"serialNumber\": \"%.*s\","
                        "       \"status\": \"provisioned\""
                        "   },"
                        "   \"reported\": {"
                        "       \"deviceType\": \"IoTDevice\","
                        "       \"serialNumber\": \"%.*s\","
                        "       \"status\": \"online\""
                        "   }"
                        "} }",
                        ( int ) gConfig.serial_len, gConfig.serial,
                        ( int ) gConfig.serial_len, gConfig.serial );

    if( ( written <= 0 ) || ( written >= SHADOW_BUFFER_SIZE ) )
    {
        LogError( ( "Failed to build shadow update payload. Written=%d", written ) );
        return;
    }

    /* Publish the shadow update */
    result = PublishToTopic( shadowUpdateTopic,
                             ( uint16_t ) strlen( shadowUpdateTopic ),
                             shadowPayload,
                             ( size_t ) written );

    if( !result )
    {
        LogError( ( "Failed to publish shadow update." ) );
    }
    else
    {
        LogInfo( ( "🌫️  Shadow update request published." ) );
    }
}


bool loadConfig(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
    {
        printf("❌ Could not open config file: %s\n", path);
        return false;
    }

    char buffer[1024];
    size_t len = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[len] = '\0';  // Null terminate JSON
    fclose(fp);

    JSONStatus_t result;
    char *value;
    size_t valueLen;

    // endpoint
    result = JSON_SearchT(buffer, len, "endpoint", strlen("endpoint"), &value, &valueLen, NULL);
    if (result == JSONSuccess)
    {
        size_t copyLen = (valueLen < sizeof(gConfig.endpoint) - 1)
                    ? valueLen
                    : sizeof(gConfig.endpoint) - 1;
        strncpy(gConfig.endpoint, value, valueLen);
        gConfig.endpoint[valueLen] = '\0';
        gConfig.endpoint_len = (int)copyLen;
    }
    else
    {
        printf("Missing key: endpoint\n");
        gConfig.endpoint[0] = '\0';
        gConfig.endpoint_len = 0;
    }

    gConfig.endpoint_len = (int)strlen(gConfig.endpoint);

    // port
    result = JSON_SearchT(buffer, len, "port", strlen("port"), &value, &valueLen, NULL);
    if (result == JSONSuccess)
    {
        char tmp[16];
        strncpy(tmp, value, valueLen);
        tmp[valueLen] = '\0';
        gConfig.port = atoi(tmp);
    }

    // root_ca
    result = JSON_SearchT(buffer, len, "root_ca", strlen("root_ca"), &value, &valueLen, NULL);
    if (result == JSONSuccess)
    {
        strncpy(gConfig.root_ca, value, valueLen);
        gConfig.root_ca[valueLen] = '\0';
    }

    // claim_cert
    result = JSON_SearchT(buffer, len, "claim_cert", strlen("claim_cert"), &value, &valueLen, NULL);
    if (result == JSONSuccess)
    {
        strncpy(gConfig.claim_cert, value, valueLen);
        gConfig.claim_cert[valueLen] = '\0';
    }

    // claim_key
    result = JSON_SearchT(buffer, len, "claim_key", strlen("claim_key"), &value, &valueLen, NULL);
    if (result == JSONSuccess)
    {
        strncpy(gConfig.claim_key, value, valueLen);
        gConfig.claim_key[valueLen] = '\0';
    }

    // template
    result = JSON_SearchT(buffer, len, "template", strlen("template"), &value, &valueLen, NULL);
    if (result == JSONSuccess)
    {
        strncpy(gConfig.template_name, value, valueLen);
        gConfig.template_name[valueLen] = '\0';
    }

    // serial
    result = JSON_SearchT(buffer, len, "serial", strlen("serial"), &value, &valueLen, NULL);
    if (result == JSONSuccess)
    {
        strncpy(gConfig.serial, value, valueLen);
        gConfig.serial[valueLen] = '\0';
    }

    gConfig.serial_len = strlen(gConfig.serial);

    result = JSON_SearchT(buffer, len, "prov_cert_path", strlen("prov_cert_path"), &value, &valueLen, NULL);
    if (result == JSONSuccess)
    {
        strncpy(gConfig.prov_cert_path, value, valueLen);
        gConfig.prov_cert_path[valueLen] = '\0';
    }

    result = JSON_SearchT(buffer, len, "prov_key_path", strlen("prov_key_path"), &value, &valueLen, NULL);
    if (result == JSONSuccess)
    {
        strncpy(gConfig.prov_key_path, value, valueLen);
        gConfig.prov_key_path[valueLen] = '\0';
    }

    // ✅ Print the loaded config
    printf("=== Config Loaded ===\n");
    printf("Endpoint            : %s\n", gConfig.endpoint);
    printf("Endpoint Length     : %d\n", gConfig.endpoint_len);
    printf("Port                : %d\n", gConfig.port);
    printf("Root CA             : %s\n", gConfig.root_ca);
    printf("Claim Cert          : %s\n", gConfig.claim_cert);
    printf("Claim Key           : %s\n", gConfig.claim_key);
    printf("Template            : %s\n", gConfig.template_name);
    printf("Serial              : %s\n", gConfig.serial);
    printf("Serial Length       : %d\n", gConfig.serial_len);
    printf("Prov Cert           : %s\n", gConfig.prov_cert_path);
    printf("Prov Key            : %s\n", gConfig.prov_key_path);
    printf("=====================\n");

    return true;
}