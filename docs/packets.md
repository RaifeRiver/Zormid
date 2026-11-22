# Data Types

| Type            | Description                                                                         |
|-----------------|-------------------------------------------------------------------------------------|
| `string`        | `uint32` length followed by content                                                 |
| `x[]`           | Array of type `x` defined as `uint32` length followed by the data                   |
| `user_id`       | 12 byte user ID                                                                     |
| `status`        | `uint32` enum<br>`0` - Offline<br>`1` - Online                                      |
| `user`          | id: `user_id`<br>username: `string`<br>avatar: `string`<br>status: `status`         |
| `channel_id`    | 20 byte channel ID                                                                  |
| `channel`       | id: `channel_id`<br>name: `string`                                                  |
| `message`       | from: `user_id`<br>content: `string`<br>username: `string`<br>timestamp: `int64`    |
| `file_id`       | 20 byte SHA1 hash of the file contents                                              |
| `upload_status` | `uint8` enum<br>`0` - Upload<br>`1` - File too large<br>`2` - File already uploaded |
| `session_id`    | 16 byte session ID                                                                  |

# Server To Client

| ID    | Data                                                                                                                         | Description                                                                    |
|-------|------------------------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------|
| `0x0` | username: `string`<br>avatar_url: `string`                                                                                   | Client successfully authenticated                                              |
| `0x1` | friends: `user[]`                                                                                                            | Response to friend list request or sent when the friend list is modified       |
| `0x2` | user_id: `user_id`<br>username: `string`                                                                                     | Response to username request                                                   |
| `0x3` | user_id: `user_id`<br>channels: `channel[]`                                                                                  | Response to channel list request or sent when the user's channel list changes  |
| `0x4` | from: `user_id`<br>channel: `channel_id`<br>content: `string`<br>username: `string`<br>index: `uint32`<br>timestamp: `int64` | Received message                                                               |
| `0x5` | channel_id: `channel_id`<br>channel_name: `string`<br>message_count: `uint32`<br>users: `user[]`                             | Response to channel info request or when the channel info changes              |
| `0x6` | channel_id: `channel_id`<br>first_message: `uint32`<br>message_count: `uint32`<br>messages: `message[]`                      | Response to channel message request                                            |
| `0x7` | file_id: `file_id`<br>status: `upload_status`                                                                                | Response to file upload request                                                |
| `0x8` | sessions: `string[]`                                                                                                         | Response to session info request, each string is the user agent of the session |
| `0x9` | user: `user`                                                                                                                 | Response to user info request or sent when a user's info changes               |
| `0xA` | channel_id: `channel_id`<br>channel_name: `string`                                                                           | User added to channel                                                          |
| `0xB` | channel_id: `channel_id`                                                                                                     | User removed from channel                                                      |

# Client To Server

| ID    | Data                                                         | Description                                                            |
|-------|--------------------------------------------------------------|------------------------------------------------------------------------|
| `0x0` | user_id: `user_id`<br>session_id: `session_id`               | Attempts to authenticate the connection                                |
| `0x1` | N/A                                                          | Requests the user's friend list from the server                        |
| `0x2` | user_id: `user_id`                                           | Requests the username of user with id `user_id`                        |
| `0x3` | user_id: `user_id`                                           | Accepts friend request from user with id `user_id`                     |
| `0x4` | user_id: `user_id`                                           | Requests list of channels that user with id `user_id` is in            |
| `0x5` | channel_id: `channel_id`<br>content: `string`                | Sends a message                                                        |
| `0x6` | channel_id: `channel_id`                                     | Requests channel info for a specific channel                           |
| `0x7` | channel_name: `string`<br>users: `user_id[]`                 | Creates a channel, does not implicitly contain the current user        |
| `0x8` | channel_id: `channel_id`<br>first_message: `uint32`          | Requests 50 messages from a specified start index                      |
| `0x9` | username: `string`                                           | Changes the current user's username                                    |
| `0xA` | channel_id: `channel_id`<br>users: `user_id[]`               | Replaces the channel's entire user list with the specified             |
| `0xB` | file_id: `file_id`<br>file_size: `uint32`                    | Requests to upload a file with given id and size                       |
| `0xC` | file_id: `file_id`<br>file_name: `string`<br>data: `uint8[]` | Uploads a file, should only be done after successful upload request    |
| `0xD` |                                                              | Requests list of the user agents of the current user's sessions        |
| `0xE` | user_agent: `string`                                         | Logs out sessions with the specified user agent                        |
| `0xF` | avatar_url: `string`                                         | Sets the current user's avatar URL, upload the file first if necessary |