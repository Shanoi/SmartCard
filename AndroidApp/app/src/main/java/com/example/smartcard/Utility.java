package com.example.smartcard;

public class Utility {

    /**
     * Function used to concatenate byte arrays to return them to the card reader.
     * @param arrays The arrays to concatenate
     * @return The arrays concatenated
     */
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

    /**
     * Function to convert a byte array into hexadecimal string
     * @param bytes
     * @return
     */
    public static String convertToString(byte[] bytes) {

        StringBuilder sb = new StringBuilder();
        sb.append("[ ");

        for (byte b : bytes) {

            sb.append(String.format("0x%02X ", b));

        }

        sb.append("]");

        return sb.toString();

    }

    public static byte[] hexStringToByteArray(String s) {
        byte[] b = new byte[s.length() / 2];
        for (int i = 0; i < b.length; i++) {
            int index = i * 2;
            int v = Integer.parseInt(s.substring(index, index + 2), 16);
            b[i] = (byte) v;
        }
        return b;
    }

}
