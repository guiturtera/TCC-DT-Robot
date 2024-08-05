import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    print("Conectado com sucesso ao broker")
    # Lista de tópicos a limpar
    topics = ["/TEF/DeviceRoboArm001/attrs"]
    for topic in topics:
        client.publish(topic, payload="", retain=False)
    client.disconnect()

client = mqtt.Client()
client.on_connect = on_connect

client.connect("34.201.54.193", 1883, 60)
client.loop_forever()
