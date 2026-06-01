# BDS Chatbot

A [LeviLamina](https://github.com/LiteLDev/LeviLamina) plugin for Minecraft Bedrock Dedicated Server that lets players chat with an AI assistant in-game.

## How it works

Players prefix their message with `ai ` to ask a question:

```
ai where is Alex?
ai what time is it?
ai how many players are online?
```

The plugin figures out what server data is needed (player positions, online count, world time, etc.), collects it, and forwards everything to an AI backend to generate a response.

## Config

Located at `plugins/BDS_Chatbot/config.json` after first run.

```json
{
  "server_name": "Dedicated Server",
  "server_ip": "play.example.com",
  "server_port": 19132,
  "version": 1
}
```

## Dependencies

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) (with OpenSSL)

## Build

```bash
xmake build
```