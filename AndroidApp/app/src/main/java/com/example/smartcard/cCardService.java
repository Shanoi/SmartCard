package com.example.smartcard;

import android.nfc.cardemulation.HostApduService;
import android.os.Bundle;

public class cCardService extends HostApduService {
    @Override
    public byte[] processCommandApdu(byte[] commandApdu, Bundle extras) {
        return new byte[]{(byte) 0x90, (byte) 0x00};
    }

    @Override
    public void onDeactivated(int reason) {

    }
}
