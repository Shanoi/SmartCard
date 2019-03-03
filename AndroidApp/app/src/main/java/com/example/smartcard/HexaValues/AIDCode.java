package com.example.smartcard.HexaValues;

/**
 * Class to have the AID code inside the application
 */
public class AIDCode {

    //        <aid-filter android:name="D2 76 00 00 85 01 01" />
    public static final byte[] AID1 = new byte[]{(byte) 0xD2, (byte) 0x76, (byte) 0x00, (byte) 0x00, (byte) 0x85, (byte) 0x01, (byte) 0x01};

    //        <aid-filter android:name="D2 76 00 00 85 01" />
    public static final byte[] AID2 = new byte[]{(byte) 0xD2, (byte) 0x76, (byte) 0x00, (byte) 0x00, (byte) 0x85, (byte) 0x01};

    //        <aid-filter android:name="D2 76 00 00 85" />
    public static final byte[] AID3 = new byte[]{(byte) 0xD2, (byte) 0x76, (byte) 0x00, (byte) 0x00, (byte) 0x85};

}
