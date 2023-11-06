const led1_btn = document.getElementById('led1_btn');
const led2_btn = document.getElementById('led2_btn');
const led3 = document.getElementById('led3');

//Initialize variables for maintaining LED elements states
let led1_2_readState;
let led3_readValue;

let led1_current_state = false;
let led2_current_state = false;

// Update the status of LED (button) elements based on the recieved MQTT message from the button event listeners 
// Save the last state of the LED to the local database; when the ESP32 client is connects, it will send the value from the last state of LED
function led1_2_updateState() {

    switch (led1_2_readState) {
        case 'LED1ON':
            led1_btn.innerHTML = 'LED 1: ON';
            led1_btn.classList.remove('btn_off');
            led1_current_state = true;
            // localStorage.setItem('led1_state', 'LED1ON');
            break;

        case 'LED1OFF':
            led1_btn.innerHTML = 'LED 1: OFF';
            led1_btn.classList.add('btn_off');
            led1_current_state = false;
            // localStorage.setItem('led1_state', 'LED1OFF');
            break;

        case 'LED2ON':
            led2_btn.innerHTML = 'LED 2: ON';
            led2_btn.classList.remove('btn_off');
            led2_current_state = true;
            // localStorage.setItem('led2_state', 'LED2ON');
            break;

        case 'LED2OFF':
            led2_btn.innerHTML = 'LED 2: OFF';
            led2_btn.classList.add('btn_off');
            led2_current_state = false;
            // localStorage.setItem('led2_state', 'LED2OFF');
            break;
    }
}
//Update the state and appearance of LED3 based on the received MQTT message.
//Save the last state(value) of LED to the local database
function led3_updateState() {
    led3.innerHTML = `LED3: ${led3_readValue}%`;
    led3.style.filter = `saturate(${led3_readValue}%)`;
    led3.style.boxShadow = `0 0px ${led3_readValue / 5}px 0 rgba(116, 79, 168, ${led3_readValue})`;
    // localStorage.setItem('led3_state', led3_readValue);
}

//*************MQTT Instance************** */

const clientId = 'mqttjs_' + Math.random().toString(16).substr(2, 8)
const host = 'ws://broker.emqx.io:8083/mqtt'
const options = {
    keepalive: 60,
    clientId: clientId,
    username: 'yuenicdao',
    password: 'yuenicdao',
    protocolId: 'MQTT',
    protocolVersion: 4,
    clean: false,
    reconnectPeriod: 4000,
    connectTimeout: 30 * 1000,
}
console.log('Connecting mqtt client')

const client = mqtt.connect(host, options)

//********************MQTT Event Handlers********************************* */

client.on('error', (err) => {
    console.log('Connection error: ', err)
    client.end()
})
client.on('reconnect', () => {
    console.log('Reconnecting...')
})

//When connected to the MQTT server, subscribe to the topics
//Publish the last state of LEDS from the local database to set the recent LED states when the HTML client connects
client.on('connect', () => {
    console.log(`Client connected: ${clientId}`)
    // Subscribe
    client.subscribe(['led1_2_status', 'led3_status'], { qos: 0 })
    // client.publish('led1_2_status', localStorage.getItem('led1_state'), { qos: 0, retain: true })
    // client.publish('led1_2_status', localStorage.getItem('led2_state'), { qos: 0, retain: true })
    // client.publish('led3_status', localStorage.getItem('led3_state'), { qos: 0, retain: true })
    client.publish('esp_status', 'getStatus', { qos: 0, retain: true })
})

// Receive MQTT messages and process based from the given conditions
client.on('message', (topic, message) => {
    if (topic === 'led1_2_status') {
        console.log(`Received Message: ${message.toString()} On topic: ${topic}`)
        led1_2_readState = message.toString();
        led1_2_updateState();
    } else if (topic === 'led3_status') { 
        led3_readValue = message.toString();
        led3_updateState();
    }
});


// Event listeners for LED1 and LED2 button clicks
led1_btn.addEventListener('click', () => {
    if (!led1_current_state) client.publish('led1_2_status', 'LED1ON', { qos: 0, retain: true })
    else client.publish('led1_2_status', 'LED1OFF', { qos: 0, retain: true })
    led1_current_state = !led1_current_state;
});

led2_btn.addEventListener('click', () => {
    if (!led2_current_state) client.publish('led1_2_status', 'LED2ON', { qos: 0, retain: true })
    else client.publish('led1_2_status', 'LED2OFF', { qos: 0, retain: true })
    led2_current_state = !led2_current_state;
});

//When the ESP32 Connects to the MQTT, it will send a message to notify the HTML client to retrieve the last state/value of the LEDs
// client.on('message', (topic, message) => {
//     if (topic === 'esp_status') {
//         if (message.toString() == 'isConnected') {
//             client.publish('led1_2_status', localStorage.getItem('led1_state'), { qos: 0, retain: true })
//             client.publish('led1_2_status', localStorage.getItem('led2_state'), { qos: 0, retain: true })
//             client.publish('led3_status', localStorage.getItem('led3_state'), { qos: 0, retain: true })
//         }
//     }
// });