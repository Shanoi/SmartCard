package com.example.smartcard.HexaValues;

/**
 * Class to represent the CCFile
 */
public class CCFile {

    public static final byte[] CCLEN = new byte[]{(byte) 0x00, (byte) 0x00};
    public static final byte[] MappingVersion = new byte[]{(byte) 0xff};
    public static final byte[] TRUEMLe = new byte[]{(byte) 0x00, (byte) 0x3b};
    public static final byte[] TRUEMLc = new byte[]{(byte) 0x00, (byte) 0x34};
    public static final byte[] TRUENDEFFile = new byte[]{(byte) 0x00, (byte) 0x00, (byte) 0x81, (byte) 0x01, (byte) 0x01, (byte) 0x20};

    /**
     * These fields represent the true values inside the code and not the values of the CCFile itself
     */
    public static final byte[] MLe = new byte[]{(byte) 0x00, (byte) 0xff};
    public static final byte[] MLc = new byte[]{(byte) 0x00, (byte) 0xff};
    public static final byte[] NDEFFile = new byte[]{(byte) 0x00, (byte) 0x00, (byte) 0x81, (byte) 0x01, (byte) 0x01, (byte) 0x20};


    /**
     * Method to return all the fields of the CCFile
     * @return The CCFile in hexadecimal
     */
    public static byte[] ccFile() {

        return new byte[]{CCLEN[0], CCLEN[1],
                MappingVersion[0],
                TRUEMLe[0], TRUEMLe[1],
                TRUEMLc[0], TRUEMLc[1],
                TRUENDEFFile[0], TRUENDEFFile[1], TRUENDEFFile[2], TRUENDEFFile[3], TRUENDEFFile[4], TRUENDEFFile[5]};

    }

}
