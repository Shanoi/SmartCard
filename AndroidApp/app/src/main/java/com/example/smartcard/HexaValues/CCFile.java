package com.example.smartcard.HexaValues;

public class CCFile {

    public static final byte[] CCLEN = new byte[]{(byte) 0x00, (byte) 0x00};
    public static final byte[] MappingVersion = new byte[]{(byte) 0xff};
    public static final byte[] MLe = new byte[]{(byte) 0x00, (byte) 0xff};
    public static final byte[] MLc = new byte[]{(byte) 0x00, (byte) 0xff};
    public static final byte[] NDEFFile = new byte[]{(byte) 0x00, (byte) 0x00, (byte) 0x81, (byte) 0x01, (byte) 0x0f, (byte) 0xff};

    public static byte[] ccFile() {

        return new byte[]{CCLEN[0], CCLEN[1],
                MappingVersion[0],
                MLe[0], MLe[1],
                MLc[0], MLc[1],
                NDEFFile[0], NDEFFile[1], NDEFFile[2], NDEFFile[3], NDEFFile[4], NDEFFile[5]};

    }


}
