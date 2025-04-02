UNEX_LIB_AVAILABLE = True
try:
    import unex_lib_V2X as unex_lib_V2X
except ModuleNotFoundError:
    UNEX_LIB_AVAILABLE = False


class UnexLib:
    """
    BTP Router for Unex Devices.

    A class that provides an interface for sending and receiving data using the unex_lib_V2X module.

    Message Types and Corresponding Parameters
    ------------------------------------------

    | Message Type | Caster ID | BTP Type | BTP Port | Transport Type |
    |--------------|-----------|----------|----------|----------------|
    | CAM          |     0     |    B     |   2001   |      SHB       |
    | DENM         |     1     |    B     |   2002   |   SHB / GBC    |
    | MAPEM        |     2     |    B     |   2003   |      SHB       |
    | SPATEM       |     3     |    B     |   2004   |      SHB       |
    | IVIM         |     4     |    B     |   2006   |      SHB       |
    | RTCMEM       |     5     |    B     |   2013   |      SHB       |
    | VAM          |     6     |    B     |   2018   |      SHB       |

    """

    def __init__(self):
        if not UNEX_LIB_AVAILABLE:
            raise ModuleNotFoundError("Unex Lib Not Installed")

    def request(self, payload: bytes, caster_id: int, is_secured: int):
        """
        Sends a data payload to a specified caster ID.

        Parameters
        ----------
        payload : bytes
            The data payload to be sent.
        caster_id : int
            The ID of the caster to which the data is to be sent.
        is_secured : int
            A flag indicating whether the message is secured (0 for unsecured, 1 for secured).
        """
        unex_lib_V2X.send(payload, caster_id, is_secured)

    def receive(self, caster_id: int) -> bytes:
        """
        Receives data from a specified caster ID.

        Parameters
        ----------
        caster_id : int
            The ID of the caster from which to receive data.

        Returns
        -------
        bytes
            The received data payload.
        """
        rx_payload = unex_lib_V2X.receive(caster_id)
        return rx_payload
