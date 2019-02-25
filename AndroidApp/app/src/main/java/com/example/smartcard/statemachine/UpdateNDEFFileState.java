package com.example.smartcard.statemachine;

import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;

import static com.example.smartcard.HexaValues.APDUOffset.LC;
import static com.example.smartcard.HexaValues.APDUOffset.P1;
import static com.example.smartcard.HexaValues.APDUOffset.P2;
import static com.example.smartcard.HexaValues.CCFile.MLc;
import static com.example.smartcard.HexaValues.ReturnCode.LC_INCORRECT;
import static com.example.smartcard.HexaValues.ReturnCode.OK_CODE;
import static com.example.smartcard.HexaValues.ReturnCode.P1_2_INCORRECT_READUPDATE;
import static com.example.smartcard.Utility.print;

public class UpdateNDEFFileState implements ReadingState {

    private static final String TAG_APDU = "UPDATE NDEF FILE";
    private boolean firstUpdate = true;

    private int lengthToWrite = 0;

    @Override
    public byte[] apply(State state, byte[] commandApdu) {

        Log.d(TAG_APDU, "/");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");
        Log.d(TAG_APDU, "//////////UPDATE FILE////////////////////////");
        Log.d(TAG_APDU, "/////////////////////////////////////////////");

        int offset = (commandApdu[P1] & 0xFF) << 8 + commandApdu[P2] & 0xFF;
        int incomingLC = commandApdu[LC] & 0xFF;

        int a = (int) (MLc[0] << 8);
        int b = (int) (MLc[1]);

        int mlc = a + b;

        if (incomingLC > mlc) {
            state.setFileSelected(false);
            state.setState(new InitialState());
            Log.d(TAG_APDU, "LC INCORRECT");
            return LC_INCORRECT;
        }

        if (offset + incomingLC > state.getValidContentLength()) {
            state.setFileSelected(false);
            state.setState(new InitialState());
            Log.d(TAG_APDU, "P1 P2 INCORRECT READ / UPDATE INCORRECT");
            return P1_2_INCORRECT_READUPDATE;
        }

        if (firstUpdate) {
            lengthToWrite = (commandApdu[5] & 0xFF) << 8 + commandApdu[6] & 0xFF;
            lengthToWrite += 2;
            int aa = (commandApdu[5] & 0xFF) << 8;
            int bb =  commandApdu[6] & 0xFF;
            Log.d(TAG_APDU, "5 = " + aa + "   " + bb);
            Log.d(TAG_APDU, "UPDATE LENGTH TO WRITE : " + lengthToWrite);
            state.setFileContent(new byte[lengthToWrite]);
            firstUpdate = false;
        }

        byte[] data = new byte[commandApdu.length - 5];

        System.arraycopy(commandApdu, 5, data, 0, incomingLC);

        state.setFileContent(data, offset);

        if (offset + incomingLC == lengthToWrite) {
            state.setState(new InitialState());
            state.setFileSelected(false);

            File file = new File(state.getContext().getCacheDir(), "NDEFFile");
            FileOutputStream outputStream;

            try {
                outputStream = new FileOutputStream(file, false);
                outputStream.write(state.getFileContent());
                outputStream.close();
                Log.d(TAG_APDU, "FILE HAS BEEN WRITTEN");
            } catch (Exception e) {
                e.printStackTrace();
            }

            lengthToWrite = 0;
            firstUpdate = false;

        }

//        byte[] content = new byte[length];
//        byte[] file = state.getFileContent();
//
//        for (int i = offset; i < length; i++) {
//
//            content[i - offset] = file[i];
//
//        }
//
//
//        if (state.getValidContentLength() == offset + length) {
//            Log.d(TAG_APDU, "Finished Reading File");
//            state.setState(new InitialState());
//            state.setFileSelected(false);
//        }

        Log.d(TAG_APDU, "OK CODE DATA WRITTEN");
        return OK_CODE;

    }
}
