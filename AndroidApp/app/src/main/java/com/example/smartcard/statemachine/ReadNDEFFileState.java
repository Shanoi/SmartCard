package com.example.smartcard.statemachine;

import android.util.Log;

import static com.example.smartcard.HexaValues.APDUOffset.LC;
import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.CCFile.MLe;
import static com.example.smartcard.HexaValues.ReturnCode.LE_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OFFSET_LE_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.Utility.concateByteArray;
import static com.example.smartcard.Utility.print;

public class ReadNDEFFileState implements ReadingState {

    private static final String TAG_APDU = "ReadNDEFFile State APDU";

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        Log.d(TAG_APDU, "/");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");
        Log.d(TAG_APDU, "//////////READ NDEF FILE/////////////////////");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");

        int offset = (int) ((commandApdu[P1] & 0xFF) << 8) + (int) (commandApdu[P2] & 0xFF);

        int length = commandApdu[LC] & 0xFF;

        Log.d(TAG_APDU, "OFFSET + LEN = " + offset + " " + length);
        Log.d(TAG_APDU, "Valid Length = " + state.getValidContentLength());

        if (offset + length > state.getValidContentLength()) {

            state.setFileSelected(false);
            state.setState(new InitialState());
            Log.d(TAG_APDU, "OFFSET LE INCORRECT");
            return OFFSET_LE_INCORRECT;

        }

        int a = (int) (MLe[0] << 8);
        int b = (int) (MLe[1]);

        int mle = a + b;

        if (length > mle) {
            Log.d(TAG_APDU, "LE INCORRECT");
            return LE_INCORRECT;
        }

        byte[] content = new byte[length];
        byte[] file = state.getFileContent();

        for (int i = offset; i < length + offset; i++) {
            Log.d(TAG_APDU, "COPY " + offset + " " + i);
            Log.d(TAG_APDU, "" + print(new byte[] {file[i]}));
            content[i - offset] = file[i];
        }

        if (state.getValidContentLength() == offset + length) {
            Log.d(TAG_APDU, "Finished Reading File");
            state.setState(new InitialState());
            state.setFileSelected(false);
        }

        //Log.d(TAG_APDU, "OK CODE " + print(concateByteArray(content, OK_CODE)));
        Log.d(TAG_APDU, "OK CODE ");
        return concateByteArray(content, OK_CODE);

    }
}
