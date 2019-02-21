package com.example.smartcard.HexaValues;

public class ReturnCode {

    public static final byte[] OK_CODE = new byte[]{(byte) 0x90, (byte) 0x00};

    public static final byte[] CLA_UNKNOWN = new byte[]{(byte) 0x6E, (byte) 0x00};
    public static final byte[] INS_UNKNOWN = new byte[]{(byte) 0x6D, (byte) 0x00};
    public static final byte[] LC_INCORRECT = new byte[]{(byte) 0x67, (byte) 0x00};
    public static final byte[] LE_INCORRECT = new byte[]{(byte) 0x6C, (byte) 0x00};
    public static final byte[] AID_LID_UNKNOWN = new byte[]{(byte) 0x6A, (byte) 0x82};
    public static final byte[] NO_COMPLIANT_STATE = new byte[]{(byte) 0x69, (byte) 0x86};
    public static final byte[] P1_2_INCORRECT_SELECT = new byte[]{(byte) 0x6A, (byte) 0x86};
    public static final byte[] P1_2_INCORRECT_READUPDATE = new byte[]{(byte) 0x6B, (byte) 0x00};
    public static final byte[] OFFSET_LC_INCORRECT = new byte[]{(byte) 0x6A, (byte) 0x87};
    public static final byte[] OFFSET_LE_INCORRECT = new byte[]{(byte) 0x6C, (byte) 0x00};

}
