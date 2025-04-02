from .unex_btp_router import UnexRouter
from v2xreferencekit.btp.router import Router as BTPRouter
from v2xreferencekit.facilities.ca_basic_service.ca_basic_service import (
    CooperativeAwarenessBasicService,
)
from v2xreferencekit.facilities.ca_basic_service.cam_transmission_management import (
    VehicleData,
)
from v2xreferencekit.utils.static_location_service import ThreadStaticLocationService

import argparse
import signal
import sys


def parse_mac_address_to_int(mac_address: str):
    mac_address = mac_address.split(":")
    mac_address = [int(x, 16) for x in mac_address]
    return bytes(mac_address)


def check_valid_mac_address(mac_address: bytes):
    if len(mac_address) != 6:
        raise ValueError("Invalid MAC address length")
    if mac_address[0] & 0x01:
        raise ValueError("Invalid MAC address, multicast bit set")


def check_station_id(station_id: int):
    if station_id < 0 or station_id > 4294967295:
        raise ValueError("Invalid station ID")


def check_station_type(station_type: int):
    if station_type < 0 or station_type > 15:
        raise ValueError("Invalid station type")


def check_driving_direction(driving_direction: int):
    if (
        driving_direction != "forward"
        and driving_direction != "backward"
        and driving_direction != "unavailable"
    ):
        raise ValueError("Invalid driving direction")


def check_vehicle_length(vehicle_length: int):
    if vehicle_length < 0 or vehicle_length > 1023:
        raise ValueError("Invalid vehicle length")


def check_vehicle_width(vehicle_width: int):
    if vehicle_width < 0 or vehicle_width > 62:
        raise ValueError("Invalid vehicle width")


def start_sending_test_cams(args):

    location_service = ThreadStaticLocationService()

    btp_router = UnexRouter()

    vehicle_data = VehicleData()
    vehicle_data.station_id = args.station_id
    vehicle_data.station_type = args.station_type
    vehicle_data.drive_direction = args.drive_direction
    vehicle_data.vehicle_length["vehicleLengthValue"] = args.vehicle_length
    vehicle_data.vehicle_width = args.vehicle_width

    cam_service = CooperativeAwarenessBasicService(btp_router, vehicle_data)

    location_service.add_callback(
        cam_service.cam_transmission_management.location_service_callback
    )

    def signal_handler(sig, frame):
        location_service.stop_event.set()
        location_service.location_service_thread.join()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    signal.pause()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Starts a GN router and sends CAMs to the specified MAC address"
    )
    parser.add_argument(
        "--station-id",
        type=int,
        default=0,
        help="The station ID to use in the CAMs [0..4294967295]",
    )
    parser.add_argument(
        "--station-type",
        type=int,
        default=0,
        help="""The station type to use in the CAMs:
    unknown         (0),
    pedestrian      (1),
    cyclist         (2),
    moped           (3),
    motorcycle      (4),
    passengerCar    (5),
    bus             (6),
    lightTruck      (7),
    heavyTruck      (8),
    trailer         (9),
    specialVehicle  (10),
    tram            (11),
    lightVruVehicle (12),
    animal          (13),
    roadSideUnit    (15)""",
    )
    parser.add_argument(
        "--drive-direction",
        type=str,
        default="unavailable",
        help="""The drive direction to use in the CAMs. It can be:
    forward,
    backward,
    unavailable """,
    )
    parser.add_argument(
        "--vehicle-length",
        type=int,
        default=1023,
        help="The vehicle length to use in the CAMs [Unit: 0.1 meter] [0..1022]",
    )
    parser.add_argument(
        "--vehicle-width",
        type=int,
        default=62,
        help="The vehicle width to use in the CAMs [Unit: 0.1 meter] [0..61]",
    )
    args = parser.parse_args()

    check_station_id(args.station_id)
    check_station_type(args.station_type)
    check_driving_direction(args.drive_direction)
    check_vehicle_length(args.vehicle_length)
    check_vehicle_width(args.vehicle_width)

    start_sending_test_cams(args)
