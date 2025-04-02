from __future__ import annotations
from collections.abc import Callable
import threading
from v2xreferencekit.btp.service_access_point import BTPDataIndication, BTPDataRequest
from v2xreferencekit.geonet.common_header import CommonNH
from v2xreferencekit.geonet.service_access_point import (
    GNDataIndication,
    HeaderType,
    TopoBroadcastHST,
)


from v2xreferencekit.btp.router import Router as BTPRouter
from .unex_lib_API import UnexLib

PORT_MAPPING = {
    2001: 0,
    2002: 1,
    2003: 2,
    2004: 3,
    2006: 4,
    2013: 5,
    2018: 6,
}


class UnexRouter(BTPRouter):
    """
    Unex Router.

    Works using the same interface as a BTP Router.

    Attributes
    ----------
    indication_callbacks : Dict[int, Callable[[BTPDataIndication], None]]
        Dictionary of indication callbacks. The key is the port and the value is the callback.
    """

    def __init__(self) -> None:
        self.unex_lib = UnexLib()
        self.stop_event = threading.Event()
        self.threads: list[threading.Thread] = []

    def register_indication_callback_btp(
        self, port: int, callback: Callable[[BTPDataIndication], None]
    ) -> None:
        """
        Registers a callback for a given port.

        Parameters
        ----------
        port : int
            Port to register the callback for.
        callback : Callable[[BTPDataIndication], None]
            Callback to register.
        """
        receiving_thread = threading.Thread(
            target=self.receiving_loop, args=[port, callback], daemon=True
        )
        self.threads.append(receiving_thread)
        receiving_thread.start()

    def stop_receiving(self):
        self.stop_event.set()
        for tt in self.threads:
            tt.join()

    def btp_data_request(self, request: BTPDataRequest) -> None:
        """
        Handles a BTPDataRequest.

        Parameters
        ----------
        request : BTPDataRequest
            BTPDataRequest to handle.
        """
        if (
            request.btp_type == CommonNH.BTP_B
            and request.gn_packet_transport_type.header_type == HeaderType.TSB
            and request.gn_packet_transport_type.header_subtype
            == TopoBroadcastHST.SINGLE_HOP
        ):

            if request.destination_port in PORT_MAPPING:
                self.unex_lib.request(
                    request.data, PORT_MAPPING[request.destination_port], 0
                )
            else:
                raise ValueError("There is no caster for this BTP Port")
        else:
            raise ValueError("There is no Caster available for this configuration")

    def receiving_loop(self, port: int, callback: Callable[[BTPDataIndication], None]):
        while not self.stop_event.is_set():
            received_data = self.unex_lib.receive(PORT_MAPPING[port])
            btp_data_indication = BTPDataIndication()
            btp_data_indication.source_port = port
            btp_data_indication.data = received_data
            btp_data_indication.length = len(received_data)
            callback(btp_data_indication)

    def btp_b_data_indication(self, gn_data_indication: GNDataIndication) -> None:
        """
        Overwritten function that shouldn't be used
        """
        raise NotImplementedError("This method shouldn't be called")

    def btp_a_data_indication(self, gn_data_indication: GNDataIndication) -> None:
        """
        Overwritten function that shouldn't be used
        """
        raise NotImplementedError("This method shouldn't be called")

    def btp_data_indication(self, gn_data_indication: GNDataIndication) -> None:
        """
        Overwritten function that shouldn't be used
        """
        raise NotImplementedError("This method shouldn't be called")
