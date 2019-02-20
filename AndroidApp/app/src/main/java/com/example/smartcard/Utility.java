package com.example.smartcard;

public class Utility {

    public static byte[] concateByteArray(byte[]... arrays) {

        int length = 0;

        for (byte[] array : arrays) {
            length += array.length;
        }

        byte[] result = new byte[length];
        int offset = 0;

        for (byte[] array : arrays) {
            System.arraycopy(array, 0, result, offset, array.length);
            offset += array.length;
        }

        return result;

    }

}
