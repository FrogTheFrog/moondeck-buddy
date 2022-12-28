import asyncio
from enum import Enum
import aiohttp
import contextlib
import base64
import inspect
import pathlib
import ssl

from typing import Optional

import copy

BUDDY_API_VERSION = 1

from typing import Any, Dict, Literal, Type, TypeVar, get_args, get_origin, TypedDict

T = TypeVar("T")
def from_dict(output_type: Type[T], data: Dict[str, Any]) -> T:
    def is_typed_dict(data_type):
        return hasattr(data_type, "__annotations__")

    def is_dict_like(data):
        return is_typed_dict(data) or isinstance(data, dict)
    
    def is_enum_like(data_type):
        return inspect.isclass(data_type) and issubclass(data_type, Enum)

    def is_literal_like(data_type):
        return get_origin(data_type) == Literal

    def get_annotations(data_type, data):
        if is_typed_dict(data_type):
            for key, key_type in data_type.__annotations__.items():
                yield (key, key_type)
        else:
            for key in data.keys():
                yield (key, data_type[1])

    verified_data = {}
    for key, key_type in get_annotations(output_type, data):
        if key not in data:
            raise ValueError(f"Key \"{key}\" is not available in {data}.")

        actual_type = get_args(key_type) or key_type
        if is_dict_like(data[key]):
            verified_data[key] = from_dict(actual_type, data[key])
            continue
        elif is_literal_like(key_type):
            if data[key] not in actual_type:
                raise TypeError(f"Value {data[key]} of \"{key}\" does not match the valid literal value(-s) {actual_type}")
        elif is_enum_like(actual_type):
            verified_data[key] = actual_type[data[key]]
            continue
        elif not isinstance(data[key], actual_type):
            raise TypeError(f"\"{key}\" value {data[key]} is not of valid type(-s) {actual_type}")

        verified_data[key] = copy.deepcopy(data[key])

    return output_type(**verified_data) if is_typed_dict(output_type) else verified_data


class ApiVersionResponse(TypedDict):
    version: int


class PairingState(Enum):
    Paired = 0
    Pairing = 1
    NotPaired = 2


class PairingStateResponse(TypedDict):
    state: PairingState


class PcState(Enum):
    Normal = 0
    Restarting = 1
    ShuttingDown = 2

class PcStateChange(Enum):
    Restart = 0
    Shutdown = 1


class PcStateResponse(TypedDict):
    state: PcState


class ResultLikeResponse(TypedDict):
    result: bool


class StreamState(Enum):
    NotStreaming = 0
    Streaming = 1
    StreamEnding = 2


class HostInfoResponse(TypedDict):
    steamIsRunning: bool
    steamRunningAppId: int
    steamTrackedUpdatingAppId: Optional[int]
    streamState: StreamState


class BuddyRequests(contextlib.AbstractAsyncContextManager):

    def __init__(self, address: str, port: int, client_id: str, timeout: float) -> None:
        super().__init__()
        self.base_url = f"https://{address}:{port}"
        self.client_id = client_id
        self.timeout = timeout
        self.__session: Optional[aiohttp.ClientSession] = None

    async def __aenter__(self):
        timeout = aiohttp.ClientTimeout(total=self.timeout)
        headers = {"authorization": f"basic {base64.b64encode(self.client_id.encode('utf-8')).decode('utf-8')}"}

        cafile = str(pathlib.Path(__file__).parent.resolve().joinpath("resources", "ssl", "moondeck_cert.pem"))
        ssl_context = ssl.create_default_context(ssl.Purpose.SERVER_AUTH, cafile=cafile)
        ssl_context.check_hostname = False
        connector = aiohttp.TCPConnector(ssl=ssl_context)

        self.__session = await aiohttp.ClientSession(
            timeout=timeout, 
            raise_for_status=True, 
            headers=headers,
            connector=connector
        ).__aenter__()
        return self

    async def __aexit__(self, *args):
        session = self.__session
        self.__session = None
        return await session.__aexit__(*args)

    async def get_api_version(self):
        async with self.__session.get(f"{self.base_url}/apiVersion") as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(ApiVersionResponse, data)

    async def get_pairing_state(self):
        async with self.__session.get(f"{self.base_url}/pairingState/{self.client_id}") as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(PairingStateResponse, data)

    async def post_start_pairing(self, pin: int):
        data = {
            "id": self.client_id,
            "hashed_id": base64.b64encode((self.client_id + str(pin)).encode("utf-8")).decode("utf-8")
        }

        async with self.__session.post(f"{self.base_url}/pair", json=data) as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(ResultLikeResponse, data)

    async def post_abort_pairing(self):
        data = {
            "id": self.client_id
        }

        async with self.__session.post(f"{self.base_url}/abortPairing", json=data) as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(ResultLikeResponse, data)

    async def post_launch_steam_app(self, app_id: int):
        data = {
            "app_id": app_id
        }

        async with self.__session.post(f"{self.base_url}/launchSteamApp", json=data) as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(ResultLikeResponse, data)

    async def post_close_steam(self, grace_period: Optional[int]):
        data = {
            "grace_period": grace_period
        }

        async with self.__session.post(f"{self.base_url}/closeSteam", json=data) as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(ResultLikeResponse, data)

    async def get_pc_state(self):
        async with self.__session.get(f"{self.base_url}/pcState") as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(PcStateResponse, data)

    async def post_change_pc_state(self, state: PcStateChange, grace_period: int):
        data = {
            "state": state.name,
            "grace_period": grace_period
        }

        async with self.__session.post(f"{self.base_url}/changePcState", json=data) as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(ResultLikeResponse, data)

    async def post_change_resolution(self, width: int, height: int, immediate: bool):
        data = {
            "width": width,
            "height": height,
            "immediate": immediate
        }

        async with self.__session.post(f"{self.base_url}/changeResolution", json=data) as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(ResultLikeResponse, data)

    async def get_host_info(self):
        async with self.__session.get(f"{self.base_url}/hostInfo") as resp:
            data = await resp.json(encoding="utf-8")
            return from_dict(HostInfoResponse, data)


class HelloResult(Enum):
    SslVerificationFailed = "SSL certificate could not be verified!"
    Exception = "An exception was thrown, check the logs!"
    VersionMismatch = "MoonDeck/Buddy needs update!"
    Restarting = "Buddy is restarting the PC!"
    ShuttingDown = "Buddy is shutting down the PC!"
    Pairing = "MoonDeck/Buddy is pairing!"
    NotPaired = "MoonDeck/Buddy needs pairing!"
    Offline = "Buddy is offline!"


class PairingResult(Enum):
    AlreadyPaired = "Deck is already paired with Buddy!"
    BuddyRefused = "Buddy refused to start pairing. Check the logs on host!"
    Failed = "Failed to start pairing with Buddy!"


class AbortPairingResult(Enum):
    BuddyRefused = "Buddy refused to abort pairing. Check the logs on host!"
    Failed = "Failed to abort pairing with Buddy!"


class CloseSteamResult(Enum):
    BuddyRefused = "Buddy refused to close Steam. Check the logs on host!"
    Failed = "Failed to close Steam via Buddy!"


class ChangePcStateResult(Enum):
    BuddyRefused = "Buddy refused to change PC state. Check the logs on host!"
    Failed = "Failed to change PC state via Buddy!"


class LaunchSteamAppResult(Enum):
    BuddyRefused = "Buddy refused to launch Steam app. Check the logs on host!"
    Failed = "Failed to launch Steam app via Buddy!"

class ChangeResolutionResult(Enum):
    BuddyRefused = "Buddy refused to change resolution. Check the logs on host!"
    Failed = "Failed to change resolution via Buddy!"

class GetHostInfoResult(Enum):
    Failed = "Failed to get host info via Buddy!"


class BuddyClient(contextlib.AbstractAsyncContextManager):

    def __init__(self, address: str, port: int, client_id: str, timeout: float) -> None:
        super().__init__()
        self.address = address
        self.port = port
        self.client_id = client_id
        self.timeout = timeout
        self.__requests: Optional[BuddyRequests] = None
        self.__hello_was_ok = False

    async def __aenter__(self):
        self.__requests = await BuddyRequests(self.address, self.port, self.client_id, self.timeout).__aenter__()
        return self

    async def __aexit__(self, *args):
        requests = self.__requests
        self.__requests = None
        return await requests.__aexit__(*args)

    async def _try_request(self, request, fallback_value):
        try:
            return await request
        except aiohttp.ServerTimeoutError as e:
            print(e)
            #logger.debug(f"Timeout while executing request: {e}")
            return fallback_value
        except aiohttp.ServerDisconnectedError as e:
            print(e)
            # logger.debug(f"Connection error while executing request: {e}")
            return fallback_value
        except aiohttp.ClientSSLError:
            #logger.exception("Request failed: SSL Verification.")
            return HelloResult.SslVerificationFailed
        except aiohttp.ClientError as e:
            # logger.exception(f"Client error while executing request")
            return HelloResult.Exception
        except Exception as e:
            #logger.exception("Request failed: unknown expection raised.")
            return HelloResult.Exception

    async def say_hello(self):
        async def request():
            resp = await self.__requests.get_api_version()
            if resp["version"] != BUDDY_API_VERSION:
                return HelloResult.VersionMismatch

            resp = await self.__requests.get_pairing_state()
            if resp["state"] == PairingState.NotPaired:
                return HelloResult.NotPaired
            elif resp["state"] == PairingState.Pairing:
                return HelloResult.Pairing

            resp = await self.__requests.get_pc_state()
            if resp["state"] == PcState.Restarting:
                return HelloResult.Restarting
            elif resp["state"] == PcState.ShuttingDown:
                return HelloResult.ShuttingDown
            
            return None

        if self.__hello_was_ok:
            return

        result = await self._try_request(request(), HelloResult.Offline)
        if not result:
            self.__hello_was_ok = True

        return result

    async def start_pairing(self, pin: int):
        async def request():
            result = await self.say_hello()
            if not result:
                return PairingResult.AlreadyPaired
            elif result != HelloResult.NotPaired:
                return result

            resp = await self.__requests.post_start_pairing(pin)
            if not resp["result"]:
                return PairingResult.BuddyRefused

            return None

        return await self._try_request(request(), PairingResult.Failed)

    async def abort_pairing(self):
        async def request():
            result = await self.say_hello()
            if not result:
                return None
            elif result != HelloResult.Pairing:
                return result

            resp = await self.__requests.post_abort_pairing()
            if not resp["result"]:
                return AbortPairingResult.BuddyRefused

            return None

        return await self._try_request(request(), AbortPairingResult.Failed)

    async def launch_app(self, app_id: int):
        async def request():
            result = await self.say_hello()
            if result:
                return result

            resp = await self.__requests.post_launch_steam_app(app_id)
            if not resp["result"]:
                return LaunchSteamAppResult.BuddyRefused

            return None

        return await self._try_request(request(), LaunchSteamAppResult.Failed)

    async def close_steam(self, grace_period: Optional[int]):
        async def request():
            result = await self.say_hello()
            if result:
                return result

            resp = await self.__requests.post_close_steam(grace_period)
            if not resp["result"]:
                return CloseSteamResult.BuddyRefused

            return None

        return await self._try_request(request(), CloseSteamResult.Failed)

    async def change_pc_state(self, state: PcStateChange, grace_period: int):
        async def request():
            result = await self.say_hello()
            if result:
                return result

            resp = await self.__requests.post_change_pc_state(state, grace_period)
            if not resp["result"]:
                return ChangePcStateResult.BuddyRefused

            return None

        return await self._try_request(request(), ChangePcStateResult.Failed)

    async def change_resolution(self, width: int, height: int, immediate: bool):
        async def request():
            result = await self.say_hello()
            if result:
                return result

            resp = await self.__requests.post_change_resolution(width, height, immediate)
            if not resp["result"]:
                return ChangeResolutionResult.BuddyRefused

            return None

        return await self._try_request(request(), ChangeResolutionResult.Failed)

    async def get_host_info(self):
        async def request():
            result = await self.say_hello()
            if result:
                return result

            return await self.__requests.get_host_info()

        return await self._try_request(request(), GetHostInfoResult.Failed)


async def main():
    valid_id = "b86b2d24-f1e8-40c7-bede-786c5d585f71"
    async with BuddyClient("localhost", 59999, valid_id, 1) as buddy:
        print (await buddy.get_host_info())
        

asyncio.run(main())  # main loop
