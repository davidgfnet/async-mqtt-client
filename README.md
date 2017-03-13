Async MQTT client
=================

Simple and rather basic C++ implementation of an MQTT client (v3.1.1) for
use anywhere (embedded or full OS system). Only includes the App layer and
the user is responsible for any network plumbing. This enables the underlying
layer to be TCP, UDP, TLS or any other transport layer you can think of.

For now it should only support QoS = 0 but the idea is to implement all
the features more or less.

Includes a very simple POSIX cli tool to perform some basic testing.


